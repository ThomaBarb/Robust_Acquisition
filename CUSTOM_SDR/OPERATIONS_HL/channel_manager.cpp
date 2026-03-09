#include "channel_manager.h"
#include <cstdio>

ChannelManager::ChannelManager(const SignalParameters& params, const PCPSConfig& pcps_config)
    : params_(params), pcps_config_(pcps_config)
{}

void ChannelManager::add_channel(int prn) {
    channels_.push_back(std::make_unique<Channel>(prn, params_, pcps_config_));
    printf("[ChannelManager] added channel for PRN %d\n", prn);
}

void ChannelManager::process(const ProcessedEpoch& epoch) {
    for (auto& ch : channels_) {
        ch->process(epoch);
    }
}

int ChannelManager::num_channels() const {
    return static_cast<int>(channels_.size());
}