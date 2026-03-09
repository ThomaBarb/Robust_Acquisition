#include "prn_generator.h"
#include <cmath>
#include <stdexcept>

// G2 tap delays per PRN (GPS ICD Table 3-Ia)
// Index 0 = PRN 1, index 31 = PRN 32
static const int G2_DELAYS[32] = {
    5, 6, 7, 8, 17, 18, 139, 140, 141, 251,
    252, 254, 255, 256, 257, 258, 469, 470, 471, 472,
    473, 474, 509, 512, 513, 514, 515, 516, 859, 860,
    861, 862
};

static std::vector<int> generate_raw_code(int prn) {
    if (prn < 1 || prn > 32)
        throw std::invalid_argument("GPS PRN must be between 1 and 32");

    // Initialize shift registers to all ones
    int g1[10] = {1,1,1,1,1,1,1,1,1,1};
    int g2[10] = {1,1,1,1,1,1,1,1,1,1};

    int delay = G2_DELAYS[prn - 1];

    // Pre-advance G2 by the satellite-specific delay
    for (int i = 0; i < delay; i++) {
        int feedback = g2[9] ^ g2[8] ^ g2[7] ^ g2[5] ^ g2[2] ^ g2[1];
        for (int j = 9; j > 0; j--) g2[j] = g2[j-1];
        g2[0] = feedback;
    }

    // Generate 1023 chips
    std::vector<int> code(1023);
    for (int i = 0; i < 1023; i++) {
        // G1 output: tap 10
        int g1_out = g1[9];
        // G2 output: tap 10
        int g2_out = g2[9];

        code[i] = g1_out ^ g2_out;

        // Advance G1: feedback = tap3 XOR tap10
        int fb1 = g1[9] ^ g1[2];
        for (int j = 9; j > 0; j--) g1[j] = g1[j-1];
        g1[0] = fb1;

        // Advance G2: feedback = tap2 XOR tap3 XOR tap6 XOR tap8 XOR tap9 XOR tap10
        int fb2 = g2[9] ^ g2[8] ^ g2[7] ^ g2[5] ^ g2[2] ^ g2[1];
        for (int j = 9; j > 0; j--) g2[j] = g2[j-1];
        g2[0] = fb2;
    }

    return code;
}

std::vector<float> PRNGenerator::generate_gps_l1ca(int prn, const SignalParameters& params) {
    std::vector<int> raw_code = generate_raw_code(prn);

    printf("[PRNGenerator] fs=%.0f epoch_ms=%d N=%d\n",
       params.sampling_freq_hz,
       params.epoch_length_ms,
       params.samples_per_epoch());

    const double chip_rate_hz  = 1.023e6;
    int N = params.samples_per_epoch();

    std::vector<float> upsampled(N);
    for (int i = 0; i < N; i++) {
        // Map sample index to chip index (handles fractional chips/sample)
        int chip_idx = static_cast<int>(std::floor(i * chip_rate_hz / params.sampling_freq_hz)) % 1023;
        // Convert binary (0/1) to bipolar (+1/-1)
        upsampled[i] = (raw_code[chip_idx] == 0) ? 1.0f : -1.0f;
    }

    return upsampled;
}