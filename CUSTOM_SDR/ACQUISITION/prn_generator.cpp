#include "prn_generator.h"
#include <cmath>
#include <stdexcept>
#include <iostream>
#include <bitset>
#include <array>

// G2 tap delays per PRN (GPS ICD Table 3-Ia)
// Index 0 = PRN 1, index 31 = PRN 32
static const int G2_DELAYS[32] = {
    5, 6, 7, 8, 17, 18, 139, 140, 141, 251,
    252, 254, 255, 256, 257, 258, 469, 470, 471, 472,
    473, 474, 509, 512, 513, 514, 515, 516, 859, 860,
    861, 862
};

static std::vector<int> generate_raw_code(int prn)
{
    int32_t chip_shift = 0;
    constexpr uint32_t code_length = 1023;
    std::bitset<code_length> G1{};
    std::bitset<code_length> G2{};
    auto G1_register = std::bitset<10>{}.set();  // All true
    auto G2_register = std::bitset<10>{}.set();  // All true
    uint32_t lcv;
    uint32_t lcv2;
    uint32_t delay;
    int32_t prn_idx;
    bool feedback1;
    bool feedback2;
    bool aux;

    // G2 Delays as defined in GPS-ISD-200D
    const std::array<int32_t, 51> delays = {5 /*PRN1*/, 6, 7, 8, 17, 18, 139, 140, 141, 251, 252, 254, 255, 256, 257, 258, 469, 470, 471, 472,
        473, 474, 509, 512, 513, 514, 515, 516, 859, 860, 861, 862 /*PRN32*/,
        145 /*PRN120*/, 175, 52, 21, 237, 235, 886, 657, 634, 762,
        355, 1012, 176, 603, 130, 359, 595, 68, 386 /*PRN138*/};

    // compute delay array index for given PRN number
    if (120 <= prn && prn <= 138)
        {
            prn_idx = prn - 88;  // SBAS PRNs are at array indices 31 to 50 (offset: -120+33-1 =-88)
        }
    else
        {
            prn_idx = prn - 1;
        }

    // A simple error check
    std::vector<int> code(1023);
    if ((prn_idx < 0) || (prn_idx > 51))
        {
            return code;
        }

    // Generate G1 & G2 Register
    for (lcv = 0; lcv < code_length; lcv++)
        {
            G1[lcv] = G1_register[0];
            G2[lcv] = G2_register[0];

            feedback1 = G1_register[7] xor G1_register[0];
            feedback2 = G2_register[8] xor G2_register[7] xor G2_register[4] xor G2_register[2] xor G2_register[1] xor G2_register[0];

            for (lcv2 = 0; lcv2 < 9; lcv2++)
                {
                    G1_register[lcv2] = G1_register[lcv2 + 1];
                    G2_register[lcv2] = G2_register[lcv2 + 1];
                }

            G1_register[9] = feedback1;
            G2_register[9] = feedback2;
        }

    // Set the delay
    delay = code_length - delays[prn_idx];
    delay += chip_shift;
    delay %= code_length;

    // Generate PRN from G1 and G2 Registers
    for (lcv = 0; lcv < code_length; lcv++)
        {
            aux = G1[(lcv + chip_shift) % code_length] xor G2[delay];
            if (aux == true)
                {
                    code[lcv] = 1;
                }
            else
                {
                    code[lcv] = -1;
                }
            delay++;
            delay %= code_length;
        }

    
    // printf("ORIGINAL CODE PRN=%02d\n", prn);
    // for (uint32_t ii = 0; ii < 1023; ++ii)
    //     {
    //         printf("%d ", code[ii]);
    //     }
    // std::string us_;
    // std::cin >> us_;

    return code;
}

std::vector<float> PRNGenerator::generate_gps_l1ca(int prn, const SignalParameters& params) {
    std::vector<int> raw_code = generate_raw_code(prn);

    printf("[PRNGenerator] fs=%.0f epoch_ms=%d N=%d\n",
       params.sampling_freq_hz,
       params.epoch_length_ms,
       params.samples_per_epoch());

    int N = params.samples_per_epoch();

    std::vector<float> upsampled(N);

    constexpr int32_t codeFreqBasis = 1023000;  // chips per second
    constexpr int32_t codeLength = 1023;
    constexpr float tc = 1.0F / static_cast<float>(codeFreqBasis);  // C/A chip period in sec

    const auto samplesPerCode = static_cast<int32_t>(static_cast<double>(params.sampling_freq_hz) / (static_cast<double>(codeFreqBasis) / static_cast<double>(codeLength)));

    const float ts = 1.0F / static_cast<float>(static_cast<double>(params.sampling_freq_hz));  // Sampling period in sec
    int32_t codeValueIndex;

    for (int32_t i = 0; i < samplesPerCode; i++)
    {
        // === Digitizing ==================================================

        // --- Make index array to read C/A code values --------------------
        // The length of the index array depends on the sampling frequency -
        // number of samples per millisecond (because one C/A code period is one
        // millisecond).

        codeValueIndex = static_cast<int32_t>(std::floor(ts * static_cast<float>(i) / tc));

        // --- Make the digitized version of the C/A code -------------------
        // The "upsampled" code is made by selecting values form the CA code
        // chip array (caCode) for the time instances of each sample.
        if (i == samplesPerCode - 1)
            {
                // --- Correct the last index (due to number rounding issues)
                upsampled[i] = static_cast<float>(raw_code[codeLength - 1]);
            }
        else
            {
                upsampled[i] = static_cast<float>(raw_code[codeValueIndex]);  // repeat the chip -> upsample
            }
    }


    return upsampled;
}