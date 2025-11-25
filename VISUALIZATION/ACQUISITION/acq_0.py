import matplotlib.pyplot as plt
import plotly.graph_objects as go
import h5py
import sys

# path = sys.argv[1]
path = "/home/thomas_chachou/GNSS_SDR/CREATE_DATASET_APP/ACQU/acquisition_G_1C_ch_0_3_sat_1.mat"
# path = "/home/thomas_chachou/GNSS_SDR/acquisition_G_1C_ch_0_1_sat_1.mat"
with h5py.File(path, 'r') as h5f:
    print(h5f.keys())
    print(h5f["PRN"][:])
    print(h5f["acq_delay_samples"])
    print(h5f["acq_doppler_hz"])
    print(h5f["acq_grid"])
    grid = h5f["acq_grid"][:]

fig = go.Figure(data=[go.Surface(z=grid)])
fig.show()