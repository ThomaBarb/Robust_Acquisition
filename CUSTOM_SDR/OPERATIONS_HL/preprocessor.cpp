#include "preprocessor.h"

Preprocessor::Preprocessor(const SignalParameters& params)
    : params_(params)
{}

ProcessedEpoch Preprocessor::process(const RawEpoch& raw) {
    ProcessedEpoch epoch;
    epoch.samples            = raw.samples;
    epoch.start_sample_index = raw.start_sample_index;
    epoch.timestamp_s        = raw.timestamp_s;
    return epoch;
}