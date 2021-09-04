import numpy as np
import time
import random as rand
import os

from PIL import Image
import serial_client as con
from matplotlib import pyplot as plt

img_data = None

def on_connect(usb: con.SerialUSB):
    # Send ready message to acknowledge connection
    if not usb.handshake():
        print("Handshake failed. Trying again...")
        if not usb.handshake():
            print("Giving up")
            return
    
    # Configure
    if not usb.setup(): print("Failed to configure")
    
    # Reset
    if not usb.reset(): print("Failed to reset")

    # Stream
    col_idx = 0
    while True:
        # Send bytes
        col = img_data[:, col_idx].astype(np.ubyte)
        #col = np.ones(112, dtype=np.ubyte)*(col_idx % 2)*255
        ok = usb.stream(data=col)

        col_idx += 1
        if (col_idx == len(col)):
            col_idx = 0

def main():
    # Load images and do some basic processing first
    path = os.path.dirname(os.path.realpath(__file__)) + "\\bscan.png"
    bscan = Image.open(path).resize((112, 112)) # load and resize
    data = np.asarray(bscan, dtype="int32")
    data = data[:, :, 0].copy() # convert RGB -> grayscale, remove 4th dim (opacity?) and resize

    global img_data
    img_data = np.flipud(data)

    CFG_FREQUENCY = 10 # Ultrasound probe operational frequency [MHz]
    CFG_IMG_SCALE = 1 # Linear amplification of image
    CFG_ACQ_TIME = 8 # Acquisition time [us]
    CFG_SAMP_RATE = 100 # Sampling frequency [MHz]
    CFG_NUM_PTS_GLOBAL = 40000
    CFG_NUM_PTS_LOCAL = 3000
    CFG_MIN_T = 1500
    CFG_MAX_T = 5500
    CFG_MAX_A = 20
    CFG_SPEED_SOUND = 1550
    CFG_BSCAN_DP = True
    CFG_SELECT_PLANE = 40

    config = [CFG_FREQUENCY,      CFG_IMG_SCALE,     CFG_ACQ_TIME, CFG_SAMP_RATE,
              CFG_NUM_PTS_GLOBAL, CFG_NUM_PTS_LOCAL, CFG_MIN_T,    CFG_MAX_T,
              CFG_MAX_A,          CFG_SPEED_SOUND,   CFG_BSCAN_DP, CFG_SELECT_PLANE]

    # Find relevant port
    port = con.SerialUSB.find_port()
    if port is None:
        return
    
    print(f"Detected device port '{port}'")
    usb = con.SerialUSB(port=port, config=config)
    
    # Connect
    usb.connect(con_evt=on_connect, timeout=1)

if __name__ == "__main__": main()