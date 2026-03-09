#pragma once
#include <vector>
#include "iq_reader.h"
#include "preprocessor.h"

struct AcquisitionResult {
    bool  found;
    float doppler_hz;
    int   code_phase_samples;
    float peak_metric;
};

class PCPS; // forward declaration

class Channel {
public:
    Channel(int prn, const SignalParameters& params);

    void process(const ProcessedEpoch& epoch);
    int  prn() const;
    bool is_acquired() const;

private:
    int                prn_;
    SignalParameters   params_;
    std::vector<float> code_replica_;
    AcquisitionResult  last_result_;
    bool               acquired_;

    // will be replaced by PCPS instance once implemented
    void run_acquisition(const ProcessedEpoch& epoch);
};