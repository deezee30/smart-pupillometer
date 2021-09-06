import numpy as np
import time
import random as rand
import os
import serial_client as con

from scipy import interpolate
from PIL import Image
from matplotlib import pyplot as plt

# Configuration
CFG_FREQUENCY       = 10 # Ultrasound probe operational frequency [MHz]
CFG_IMG_SCALE       = 1 # Linear amplification of image
CFG_SAMP_RATE       = 100 # Sampling frequency [MHz]
CFG_NUM_PTS_GLOBAL  = 200
CFG_MIN_T           = 0
CFG_MAX_T           = 200
CFG_GAIN            = 50
CFG_SPEED_SOUND     = 1550
CFG_BSCAN_DP        = True
CFG_SELECT_PLANE    = 40

img_data = None

def on_connect(usb: con.SerialUSB):
    # Send ready message to acknowledge connection
    if not usb.handshake():
        print("Handshake failed. Trying again...")
        if not usb.handshake():
            print("Giving up. Are you sure the native USB port is connected?")
            return
    
    # Configure
    if not usb.setup():
        print("Failed to configure")
        return
    
    # Reset
    if not usb.reset():
        print("Failed to reset")
        return

    # Stream
    col_idx = 0
    while True:
        # Send bytes
        col = img_data[:, col_idx].astype(dtype="<i2")
        #col = np.ones(112, dtype=np.ubyte)*(col_idx % 2)*255
        if not usb.stream(data=col):
            print(f"Failed to send stream on column index {col_idx}. Stopping...")
            return

        col_idx += 1
        if (col_idx == img_data.shape[1]):
            col_idx = 0

def main():
    # Load images and do some basic processing first
    path = os.path.dirname(os.path.realpath(__file__)) + "\\test_img\\bscan.png"
    bscan = Image.open(path).resize((112, CFG_MAX_T-CFG_MIN_T)) # load and resize
    data = np.asarray(bscan)
    data = data[:, :, 0].copy() # convert RGB -> grayscale, remove 4th dim (opacity?) and resize

    global img_data
    img_data = np.flipud(data)

    config = [CFG_FREQUENCY, CFG_IMG_SCALE,    CFG_SAMP_RATE, CFG_NUM_PTS_GLOBAL,
              CFG_MIN_T,     CFG_MAX_T,        CFG_GAIN,      CFG_SPEED_SOUND, 
              CFG_BSCAN_DP,  CFG_SELECT_PLANE]

    # Find relevant port
    port = con.SerialUSB.find_port()
    if port is None:
        return
    
    print(f"Detected device port '{port}'")
    usb = con.SerialUSB(port=port, config=config)
    
    # Connect
    usb.connect(con_evt=on_connect, timeout=1)

if __name__ == "__main__": main()