#pragma once
#include <vector>
#include <complex>
#include <fftw3.h>
#include "iq_reader.h"
#include "preprocessor.h"

struct AcquisitionResult {
    bool  found;
    float doppler_hz;
    int   code_phase_samples;
    float peak_metric;
    float CN0;
};

struct PCPSConfig {
    float doppler_max_hz;
    float doppler_step_hz;
    float detection_threshold;
    int   non_coh_integrations;
};

class PCPS {
public:
    PCPS(const SignalParameters& params, const PCPSConfig& config);
    ~PCPS();

    PCPS(const PCPS&)            = delete;
    PCPS& operator=(const PCPS&) = delete;

    void set_local_code(const std::vector<float>& code_replica);
    AcquisitionResult search(const std::vector<ProcessedEpoch>& epochs);

private:
    SignalParameters params_;
    PCPSConfig       config_;
    int              N_;

    // FFT plans
    fftwf_plan plan_fwd_signal_;
    fftwf_plan plan_inv_;

    // Buffers
    fftwf_complex* buf_signal_;      // input signal (one epoch)
    fftwf_complex* buf_signal_fft_;  // FFT of doppler-wiped signal
    fftwf_complex* buf_code_fft_;    // FFT of local code (precomputed in set_local_code)
    fftwf_complex* buf_product_;     // pointwise product
    fftwf_complex* buf_corr_;        // IFFT result

    // Magnitude accumulation grid [doppler_bin][code_phase]
    std::vector<std::vector<float>> magnitude_grid_;
    std::vector<float>              doppler_grid_;

    // Doppler wipeoff carriers, precomputed [doppler_bin][sample]
    std::vector<std::vector<std::complex<float>>> doppler_wipeoffs_;

    void precompute_doppler_wipeoffs();
    float max_to_noise_floor_statistic(int& best_doppler_idx, int& best_code_phase, float& grid_max, float& noise_floor) const;
};