#pragma once
#include <vector>
#include "channel.h"
#include "preprocessor.h"

class ChannelManager {
public:
    ChannelManager(const SignalParameters& params);

    void add_channel(int prn);
    void process(const ProcessedEpoch& epoch);
    int  num_channels() const;

private:
    SignalParameters     params_;
    std::vector<Channel> channels_;
};