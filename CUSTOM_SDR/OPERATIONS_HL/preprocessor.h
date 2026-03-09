#pragma once
#include "iq_reader.h"

struct ProcessedEpoch {
    std::vector<std::complex<float>> samples;
    uint64_t start_sample_index;
    double   timestamp_s;
};

class Preprocessor {
public:
    Preprocessor(const SignalParameters& params);

    ProcessedEpoch process(const RawEpoch& raw);

private:
    SignalParameters params_;
};