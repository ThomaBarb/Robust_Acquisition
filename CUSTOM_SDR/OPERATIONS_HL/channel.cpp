#include "channel.h"
#include <cstdio>
#include <iostream>

Channel::Channel(int prn, const SignalParameters& params, const PCPSConfig& pcps_config)
    : prn_(prn), params_(params), pcps_(params, pcps_config),
      acquired_(false), epochs_needed_(pcps_config.non_coh_integrations)
{
    code_replica_ = PRNGenerator::generate_gps_l1ca(prn, params);
    pcps_.set_local_code(code_replica_);
    last_result_ = {false, 0.0f, 0, 0.0f};

    // printf("CHANNEL: PRN=%02d\n", prn);
    // for (auto& code_: code_replica_) {
    //     printf("%f ", code_);
    // }
    // std::string us_;
    // std::cin >> us_;
}


int Channel::prn() const {
    return prn_;
}

bool Channel::is_acquired() const {
    return acquired_;
}

void Channel::process(const ProcessedEpoch& epoch) 
{
    if (!acquired_) {
        run_acquisition(epoch);
    }
    // tracking will be added here once acquired
}

void Channel::run_acquisition(const ProcessedEpoch& epoch) 
{
    // Skip epochs during backoff
    if (backoff_counter_ > 0) {
        backoff_counter_--;
        return;
    }
    epoch_buffer_.push_back(epoch);

    if ((int)epoch_buffer_.size() >= epochs_needed_) {
        last_result_ = pcps_.search(epoch_buffer_);
        epoch_buffer_.clear();

        if (last_result_.found) {
            confirm_count_++;
            // printf("[Channel PRN %d] time=%f candidate found metric=%.2f confirm=%d/%d\n",
                //    prn_, epoch.timestamp_s,
                //    last_result_.peak_metric,
                //    confirm_count_, confirmations_needed_);

            if (confirm_count_ >= confirmations_needed_) {
                acquired_ = true;
                printf("[Channel PRN %d] time=%f CONFIRMED ACQUIRED CN0=%f\n",
                     prn_, epoch.timestamp_s,
                     last_result_.CN0
                    );
            }
        } else {
            confirm_count_ = 0;   // reset on any miss
            backoff_counter_ = backoff_epochs_; 
        }
    }
}