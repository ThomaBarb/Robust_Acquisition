#pragma once
#include <vector>
#include "iq_reader.h"

namespace PRNGenerator {
    // Generate upsampled GPS L1 C/A code for a given PRN (1-32)
    // Output is bipolar (+1/-1) at the receiver sampling frequency
    std::vector<float> generate_gps_l1ca(int prn, const SignalParameters& params);
}