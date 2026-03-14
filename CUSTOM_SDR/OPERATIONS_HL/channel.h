#pragma once
#include <vector>
#include <memory>
#include "iq_reader.h"
#include "preprocessor.h"
#include "../ACQUISITION/prn_generator.h"
#include "../ACQUISITION/pcps.h"
#include "../TRACKING/tracking.h"

class Channel {
public:
    Channel(int prn, const SignalParameters& params, 
        const PCPSConfig& pcps_config, const TrackingConfig& tracking_config);

    void process(const ProcessedEpoch& epoch);
    int  prn()         const;
    bool is_acquired() const;
    bool is_locked()   const;
    void run_acquisition(const ProcessedEpoch& epoch);
    void run_tracking(const ProcessedEpoch& epoch);

    Channel(const Channel&)            = delete;
    Channel& operator=(const Channel&) = delete;

private:
    int                        prn_;
    SignalParameters           params_;
    std::vector<float>         code_replica_;

    // ACQUISITION
    PCPS                        pcps_;
    AcquisitionResult           last_acq_result_;
    bool                        acquired_;
    std::vector<ProcessedEpoch> epoch_buffer_;
    int                         epochs_needed_;
    int                         confirm_count_          = 0;
    int                         confirmations_needed_   = 2;
    int                         backoff_epochs_         = 200;
    int                         backoff_counter_        = 0;

    // Tracking
    Tracking                   tracking_;
    TrackingResult             last_tracking_result_;
    int                        loss_counter_;
    int                        loss_threshold_       = 100;

};