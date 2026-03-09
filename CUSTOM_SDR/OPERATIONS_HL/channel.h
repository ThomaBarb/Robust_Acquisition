#pragma once
#include <vector>
#include "iq_reader.h"
#include "preprocessor.h"
#include "../ACQUISITION/prn_generator.h"
#include "../ACQUISITION/pcps.h"

class Channel {
public:
    Channel(int prn, const SignalParameters& params, const PCPSConfig& pcps_config);

    void process(const ProcessedEpoch& epoch);
    int  prn()         const;
    bool is_acquired() const;
    void run_acquisition(const ProcessedEpoch& epoch);

    Channel(const Channel&)            = delete;
    Channel& operator=(const Channel&) = delete;

private:
    int                        prn_;
    SignalParameters           params_;
    std::vector<float>         code_replica_;
    PCPS                       pcps_;
    AcquisitionResult          last_result_;
    bool                       acquired_;

    // Epoch accumulator for non-coherent integration
    std::vector<ProcessedEpoch> epoch_buffer_;
    int                         epochs_needed_;
};