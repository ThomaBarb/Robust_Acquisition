import h5py
import os 
from tqdm import tqdm, trange
import re
import numpy as np
import matplotlib.pyplot as plt
import imageio.v2 as imageio

# path_dir = "/home/thomas_chachou/GNSS_SDR/gnss-sdr/build/src/main/ACQU"
path_dir = "/home/thomas_chachou/GNSS_SDR/CREATE_DATASET_APP/ACQU"
files = [t for t in os.listdir(path_dir) if t.endswith('.mat')]

SAT = 1
FREQ = 2000000.
N_FILES = 0

files = [t for t in files if "sat_%d"%SAT in t]

def extract_num(fname):
    return int(re.search(r"ch_[0-9]+_([0-9]+)_sat", fname).group(1))

files = sorted(files, key=extract_num)

TIME = []
ACQ_GRID = []

for filename in tqdm(files):

    # print(filename)

    path = os.path.join(path_dir, filename)
    with h5py.File(path, 'r') as h5f:

        # print(h5f.keys())
        sample_counter = h5f["sample_counter"][:]
        # print(sample_counter)

        acq_grd = h5f["acq_grid"][:]

        # print(h5f["d_positive_acq"][:])

        TIME.append(sample_counter[0] / FREQ)
        ACQ_GRID.append(acq_grd)

        # input()

    # End with h5py.File(path, 'r') as h5f

    # input()

    N_FILES +=1

# End for file in files

TIME = np.array(TIME)
ACQ_GRID = np.stack(ACQ_GRID)
print(ACQ_GRID.shape)

frames = []
for t in trange(ACQ_GRID.shape[0]):
    plt.imshow(ACQ_GRID[t], cmap='viridis', aspect='auto') #vmin=ACQ_GRID.min(), vmax=ACQ_GRID.max(),
    plt.axis('off')
    # plt.show()
    
    # Save frame to a temporary buffer
    plt.savefig("frame.png", dpi=80, bbox_inches='tight', pad_inches=0)
    plt.close()
    
    frames.append(imageio.imread("frame.png"))
# End for t in range(ACQ_GRID.shape
imageio.mimsave("animation.gif", frames, fps=5)


dt = np.diff(TIME)

fig, ax = plt.subplots()
ax.plot(dt)
plt.show()

print(N_FILES)