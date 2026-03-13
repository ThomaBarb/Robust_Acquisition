#pragma once
#include <fstream>
#include <string>
#include <vector>
#include <complex>
#include <cstdint>

struct SignalParameters {
    double sampling_freq_hz;
    double center_freq_hz;
    int    epoch_length_ms;

    int samples_per_epoch() const {
        return static_cast<int>(sampling_freq_hz * epoch_length_ms / 1000.0);
    }
};

struct RawEpoch {
    std::vector<std::complex<float>> samples;
    uint64_t start_sample_index;
    double   timestamp_s;
};

class IQFileReader {
public:
    IQFileReader(const std::string& filename, const SignalParameters& params);
    ~IQFileReader() = default;

    bool     open();
    bool     read_epoch(RawEpoch& epoch);
    bool     is_eof() const;
    uint64_t samples_read() const;

    bool seek_to_sample(uint64_t sample_index);

private:
    std::string          filename_;
    SignalParameters     params_;
    std::ifstream        file_;
    uint64_t             sample_counter_ = 0;
    std::vector<int16_t> raw_buffer_;

    void convert_to_complex(RawEpoch& epoch);
};