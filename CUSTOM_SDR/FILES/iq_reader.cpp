#include "iq_reader.h"

IQFileReader::IQFileReader(const std::string& filename, const SignalParameters& params)
    : filename_(filename), params_(params)
{
    raw_buffer_.resize(2 * params_.samples_per_epoch());
}

bool IQFileReader::open() {
    file_.open(filename_, std::ios::binary);
    return file_.is_open();
}

bool IQFileReader::read_epoch(RawEpoch& epoch) {
    int N = params_.samples_per_epoch();

    file_.read(reinterpret_cast<char*>(raw_buffer_.data()),
               raw_buffer_.size() * sizeof(int16_t));

    std::streamsize bytes_read = file_.gcount();
    if (bytes_read < static_cast<std::streamsize>(raw_buffer_.size() * sizeof(int16_t))) {
        return false;
    }

    epoch.start_sample_index = sample_counter_;
    epoch.timestamp_s        = static_cast<double>(sample_counter_) / params_.sampling_freq_hz;
    epoch.samples.resize(N);

    convert_to_complex(epoch);

    sample_counter_ += N;
    return true;
}

void IQFileReader::convert_to_complex(RawEpoch& epoch) {
    int N = params_.samples_per_epoch();
    for (int i = 0; i < N; i++) {
        epoch.samples[i] = {
            static_cast<float>(raw_buffer_[2*i]),
            static_cast<float>(raw_buffer_[2*i + 1])
        };
    }
}

bool IQFileReader::is_eof() const {
    return file_.eof();
}

uint64_t IQFileReader::samples_read() const {
    return sample_counter_;
}