import os
import sys
import subprocess
import shutil
import requests
import h5py
import os 
from tqdm import tqdm, trange
import re
import numpy as np
import matplotlib.pyplot as plt


def getAcquisitionForSat(path_dir, SAT, FREQ = 2000000.):

    # path_dir = "/home/thomas_chachou/GNSS_SDR/CREATE_DATASET_APP/ACQU"
    files = [t for t in os.listdir(path_dir) if t.endswith('.mat')]
    files = [t for t in files if "sat_%d"%SAT in t]
    def extract_num(fname):
        return int(re.search(r"ch_[0-9]+_([0-9]+)_sat", fname).group(1))
    files = sorted(files, key=extract_num)

    TIME = []
    ACQ_GRID = []
    N_FILES = 0

    for filename in tqdm(files):

        path = os.path.join(path_dir, filename)
        with h5py.File(path, 'r') as h5f:

            # print(h5f.keys())
            sample_counter = h5f["sample_counter"][:]
            acq_grd = h5f["acq_grid"][:]

            TIME.append(sample_counter[0] / FREQ)
            ACQ_GRID.append(acq_grd)

        # End with h5py.File(path, 'r') as h5f

        N_FILES +=1

    # End for file in files

    TIME = np.array(TIME)
    ACQ_GRID = np.stack(ACQ_GRID)
    print(ACQ_GRID.shape)

        
# End of function getAcquisitionForSat


PATH_GNSS_SDR = "/home/thomas_chachou/GNSS_SDR/gnss-sdr/build/src/main/gnss-sdr"
PATH_CONF = "/home/thomas_chachou/GNSS_SDR/CREATE_DATASET_APP/conf_pvt.conf"

# Launch GNSS-SDR a first time to have a first position fix
subprocess.run([PATH_GNSS_SDR, "-c", PATH_CONF])


# url = "https://igs.bkg.bund.de/root_ftp/IGS/products/2334/igs23344.sp3.Z"
# outfile = "igs23344.sp3.Z"
# r = requests.get(url)
# r.raise_for_status()
# with open(outfile, "wb") as f:
#     f.write(r.content)
# print("Downloaded:", outfile)

# exit(0)

SATELLITES_OI = [1]
# SATELLITES_OI = [1, 2, 3, 4, 5, 6]

# Loop through satellites to test
for sat in SATELLITES_OI:

    path_template = "conf_template.conf"
    path_conf = "conf.conf"

    with open(path_template, "r") as f:
        lines = f.readlines()

    new_line = "Channel0.satellite=%d"%sat
    with open(path_conf, "w+") as f:
        for line in lines:
            if line.strip().startswith("Channel0.satellite"):
                f.write(new_line + "\n")
            else:
                f.write(line)

    # End with open(path_conf, 'w+') as f

    # Launch GNSS-SDR
    subprocess.run([PATH_GNSS_SDR, "-c", path_conf])

    getAcquisitionForSat("./ACQU", sat, FREQ = 2000000.)

# End for sat in SATELLITES_OI