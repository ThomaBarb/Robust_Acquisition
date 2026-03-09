#include "channel_manager.h"
#include <cstdio>

ChannelManager::ChannelManager(const SignalParameters& params)
    : params_(params)
{}

void ChannelManager::add_channel(int prn) {
    channels_.emplace_back(prn, params_);
    printf("[ChannelManager] added channel for PRN %d\n", prn);
}

void ChannelManager::process(const ProcessedEpoch& epoch) {
    for (Channel& ch : channels_) {
        ch.process(epoch);
    }
}

int ChannelManager::num_channels() const {
    return static_cast<int>(channels_.size());
}