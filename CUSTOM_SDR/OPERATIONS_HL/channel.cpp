#include "channel.h"
#include <cstdio>

Channel::Channel(int prn, const SignalParameters& params)
    : prn_(prn), params_(params), acquired_(false)
{
    last_result_ = {false, 0.0f, 0, 0.0f};
    // code_replica_ will be filled by PRN generator once implemented
}

int Channel::prn() const {
    return prn_;
}

bool Channel::is_acquired() const {
    return acquired_;
}

void Channel::process(const ProcessedEpoch& epoch) {
    if (!acquired_) {
        run_acquisition(epoch);
    }
    // tracking will be added here once acquired
}

void Channel::run_acquisition(const ProcessedEpoch& epoch) {
    // placeholder until PCPS is implemented
    printf("[Channel PRN %d] running acquisition at t=%.3f s\n",
           prn_, epoch.timestamp_s);
}