#include "OPERATIONS_HL/preprocessor.h"
#include <iostream>
#include "OPERATIONS_HL/channel_manager.h"

int main() 
{
    // TODO: Configuration file

    // Define parameters of the input signal
    SignalParameters params;
    params.sampling_freq_hz = 4000000.0;
    params.center_freq_hz   = 1575420000.0;
    params.epoch_length_ms  = 1;

    PCPSConfig pcps_config;
    pcps_config.doppler_max_hz       = 10000.0f;
    pcps_config.doppler_step_hz      = 250.0f;
    pcps_config.detection_threshold  = 35.f;
    pcps_config.non_coh_integrations = 1;

    // Open binary file
    std::string path = 
        // "/home/thomas_chachou/GNSS_SDR/DATA_BINARY/Galileo_5_Spoofer_Static_NoMP_TruePosition.bin";
        "/home/thomas_chachou/GNSS_SDR/DATA/2013_04_04_GNSS_SIGNAL_at_CTTC_SPAIN/2013_04_04_GNSS_SIGNAL_at_CTTC_SPAIN.dat";

    IQFileReader  reader(path, params);
    if (!reader.open()) {
        std::cerr << "Failed to open IQ file\n";
        return 1;
    }

    reader.seek_to_sample(0);

    Preprocessor  preprocessor(params);
    ChannelManager channel_manager(params, pcps_config);
    std::vector<int> prn_channels = { 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13};
    for (int& prn_: prn_channels) {
        channel_manager.add_channel(prn_);
    }

    RawEpoch       raw;
    ProcessedEpoch epoch;
    

    int iLoop=0;
    while (reader.read_epoch(raw)) {
        epoch = preprocessor.process(raw);
        channel_manager.process(epoch);


        // std::cout << "Processed " << reader.samples_read() << " samples ("
        //         << static_cast<double>(reader.samples_read()) / params.sampling_freq_hz
        //         << " s)\n";
        iLoop++;
    }
    return 0;

} // End of function main