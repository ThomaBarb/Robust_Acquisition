#include "pcps.h"
#include <cmath>
#include <algorithm>
#include <numeric>
#include <stdexcept>
#include <cstring>
#include <cstdio>
#include <iostream>
#include <gnuradio/gr_complex.h>

static const float TWO_PI = 2.0f * 3.14159265358979323846f;

float estimate_cn0(float peak_power, float noise_floor, float T_coh_s) 
{
    float snr_linear = (peak_power - noise_floor) / noise_floor;
    if (snr_linear <= 0.0f) return 0.0f;
    float cn0_linear = snr_linear / T_coh_s;
    float cn0_db_hz  = 10.0f * std::log10(cn0_linear);
    return cn0_db_hz;
}

PCPS::PCPS(const SignalParameters& params, const PCPSConfig& config)
    : params_(params), config_(config), N_(params.samples_per_epoch())
{
    // Allocate buffers
    buf_signal_     = fftwf_alloc_complex(N_);
    buf_signal_fft_ = fftwf_alloc_complex(N_);
    buf_code_fft_   = fftwf_alloc_complex(N_);
    buf_product_    = fftwf_alloc_complex(N_);
    buf_corr_       = fftwf_alloc_complex(N_);

    // Plans with explicit separate input/output buffers — no in-place ambiguity
    plan_fwd_signal_ = fftwf_plan_dft_1d(N_, buf_signal_, buf_signal_fft_, FFTW_FORWARD,  FFTW_ESTIMATE);
    plan_inv_        = fftwf_plan_dft_1d(N_, buf_product_, buf_corr_,      FFTW_BACKWARD, FFTW_ESTIMATE);

    // Build Doppler grid
    for (float d = -config_.doppler_max_hz; d <= config_.doppler_max_hz; d += config_.doppler_step_hz)
        doppler_grid_.push_back(d);

    int num_doppler_bins = static_cast<int>(doppler_grid_.size());

    // Allocate magnitude grid
    magnitude_grid_.assign(num_doppler_bins, std::vector<float>(N_, 0.0f));

    // Precompute all Doppler wipeoff carriers
    precompute_doppler_wipeoffs();

    printf("[PCPS] initialized: N=%d doppler_bins=%d\n", N_, num_doppler_bins);
}

PCPS::~PCPS() {
    fftwf_destroy_plan(plan_fwd_signal_);
    fftwf_destroy_plan(plan_inv_);
    fftwf_free(buf_signal_);
    fftwf_free(buf_signal_fft_);
    fftwf_free(buf_code_fft_);
    fftwf_free(buf_product_);
    fftwf_free(buf_corr_);
}

void PCPS::precompute_doppler_wipeoffs() {
    int num_bins = static_cast<int>(doppler_grid_.size());
    doppler_wipeoffs_.resize(num_bins, std::vector<std::complex<float>>(N_));

    for (int b = 0; b < num_bins; b++) {
        float freq = doppler_grid_[b];
        for (int i = 0; i < N_; i++) {
            float phase = -TWO_PI * freq * i / params_.sampling_freq_hz;
            doppler_wipeoffs_[b][i] = std::complex<float>(std::cos(phase), std::sin(phase));
        }
    }
}

void PCPS::set_local_code(const std::vector<float>& code_replica) {
    // Load code into a temporary FFT buffer and compute its FFT
    // Result stored in buf_code_fft_ — conjugated, matching GNSS-SDR's set_local_code
    fftwf_complex* buf_tmp = fftwf_alloc_complex(N_);
    std::string us_;

    for (int i = 0; i < N_; i++) {
        buf_tmp[i][0] = 0.0f; //code_replica[i];
        buf_tmp[i][1] = code_replica[i]; //0.0f;
        // printf("%.0f - %.0f ", buf_tmp[i][0], buf_tmp[i][1]);
    }
    // printf("\n");
    // std::cin >> us_;

    fftwf_plan plan_code = fftwf_plan_dft_1d(N_, buf_tmp, buf_code_fft_, FFTW_FORWARD, FFTW_ESTIMATE);
    fftwf_execute(plan_code);
    fftwf_destroy_plan(plan_code);
    fftwf_free(buf_tmp);

    // Conjugate in place
    for (int i = 0; i < N_; i++) {
        buf_code_fft_[i][1] = -buf_code_fft_[i][1];
        // printf("%.0f - %.0f ", buf_code_fft_[i][0], buf_code_fft_[i][1]);
    }
    // printf("\n");
    // std::cin >> us_;

    printf("[PCPS] local code FFT computed and conjugated\n");
}

float PCPS::max_to_noise_floor_statistic(int& best_doppler_idx, int& best_code_phase, float& grid_max, float& noise_floor) const 
{
    grid_max   = 0.0f;
    best_doppler_idx = 0;
    best_code_phase  = 0;

    // Find global peak — identical to GNSS-SDR's loop
    for (int b = 0; b < static_cast<int>(doppler_grid_.size()); b++) {
        for (int i = 0; i < N_; i++) {
            if (magnitude_grid_[b][i] > grid_max) {
                grid_max         = magnitude_grid_[b][i];
                best_doppler_idx = b;
                best_code_phase  = i;
            }
        }
    }

    // Noise floor — exactly matching GNSS-SDR's max_to_input_power_statistic:
    // accumulate opposite bin / effective_fft_size / 2.0 / num_noncoherent_integrations
    int num_bins = static_cast<int>(doppler_grid_.size());
    int opp_bin  = (best_doppler_idx + num_bins / 2) % num_bins;

    noise_floor = std::accumulate(magnitude_grid_[opp_bin].begin(),
                                        magnitude_grid_[opp_bin].end(), 0.0f)
                        / static_cast<float>(N_)
                        / 2.0f
                        / static_cast<float>(config_.non_coh_integrations);

    if (noise_floor < std::numeric_limits<float>::epsilon())
        return 0.0f;

    return grid_max / noise_floor;
}

AcquisitionResult PCPS::search(const std::vector<ProcessedEpoch>& epochs) 
{
    AcquisitionResult result = {false, 0.0f, 0, 0.0f};

    int num_bins = static_cast<int>(doppler_grid_.size());

    // Reset magnitude grid
    for (auto& row : magnitude_grid_)
        std::fill(row.begin(), row.end(), 0.0f);

    float norm = 1.0f / static_cast<float>(N_ * N_);

    for (int b = 0; b < num_bins; b++) 
    {
        for (int e = 0; e < config_.non_coh_integrations; e++) 
        {

            // Apply precomputed Doppler wipeoff into buf_signal_
            for (int i = 0; i < N_; i++) 
            {
                std::complex<float> wiped = epochs[e].samples[i] * doppler_wipeoffs_[b][i];
                buf_signal_[i][0] = wiped.real();
                buf_signal_[i][1] = wiped.imag();
            }

            // FFT of wiped signal → buf_signal_fft_
            fftwf_execute(plan_fwd_signal_);

            // Pointwise multiply: signal_fft * conj(code_fft)
            // conj already applied in set_local_code, so just multiply
            for (int i = 0; i < N_; i++) 
            {
                float ar = buf_signal_fft_[i][0], ai = buf_signal_fft_[i][1];
                float br = buf_code_fft_[i][0],   bi = buf_code_fft_[i][1];
                // (ar + j*ai) * (br + j*bi)  [bi is already negated]
                buf_product_[i][0] = ar * br - ai * bi;
                buf_product_[i][1] = ai * br + ar * bi;
            }

            // IFFT → buf_corr_
            fftwf_execute(plan_inv_);

            // Accumulate magnitude squared (normalized)
            for (int i = 0; i < N_; i++) 
            {
                float r  = buf_corr_[i][0] * norm;
                float im = buf_corr_[i][1] * norm;
                magnitude_grid_[b][i] += r * r + im * im;
            }

        } // End for (int e = 0; e < config_.non_coh_integrations; e++) 
    
    } // End for (int b = 0; b < num_bins; b++) 

    // Compute test statistic
    int best_doppler_idx    = 0;
    int best_code_phase     = 0;
    float peak_power        = 0.;
    float noise_floor       = 0.;
    float peak_metric = max_to_noise_floor_statistic(best_doppler_idx, best_code_phase, peak_power, noise_floor);

    result.found                = (peak_metric > config_.detection_threshold);
    result.doppler_hz           = doppler_grid_[best_doppler_idx];
    result.code_phase_samples   = best_code_phase;
    result.peak_metric          = peak_metric;

    float T_coh_s = static_cast<float>(params_.epoch_length_ms) / 1000.0f
                * config_.non_coh_integrations;
    result.CN0                  = estimate_cn0(peak_power, noise_floor, T_coh_s) ;


    // printf("[PCPS] peak_metric=%.2f doppler=%.1f Hz code_phase=%d %s\n",
    //        peak_metric, result.doppler_hz, best_code_phase,
    //        result.found ? "ACQUIRED" : "not found");

    return result;
}