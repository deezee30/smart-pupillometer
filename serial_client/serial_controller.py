import numpy as np
import time
import random as rand
import os

from PIL import Image
import serial_client as con
from matplotlib import pyplot as plt

config = [
    10, # Ultrasound probe operational frequency [MHz]
    1,  # Linear amplification of image
    8,  # Acquisition time [us]
]

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

def main(config):
    # Load images and do some basic processing first
    path = os.path.dirname(os.path.realpath(__file__)) + "\\bscan.png"
    bscan = Image.open(path).resize((112, 112)) # load and resize
    data = np.asarray(bscan, dtype="int32")
    data = data[:, :, 0].copy() # convert RGB -> grayscale, remove 4th dim (opacity?) and resize

    global img_data
    img_data = np.flipud(data)

    # Find relevant port
    port = con.SerialUSB.find_port()
    if port is None:
        return
    
    print(f"Detected device port '{port}'")
    usb = con.SerialUSB(port=port, config=config)
    
    # Connect
    usb.connect(con_evt=on_connect, timeout=1)

if __name__ == "__main__": main(config)