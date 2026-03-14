#pragma once
#include <vector>
#include <memory>
#include "channel.h"
#include "preprocessor.h"
#include "pcps.h"

class ChannelManager {
public:
    ChannelManager(const SignalParameters& params, 
        const PCPSConfig& pcps_config, const TrackingConfig& tracking_config);

    void add_channel(int prn);
    void process(const ProcessedEpoch& epoch);
    int  num_channels() const;

private:
    SignalParameters                   params_;
    PCPSConfig                         pcps_config_;
    std::vector<std::unique_ptr<Channel>> channels_;
    TrackingConfig                      tracking_config_;
};