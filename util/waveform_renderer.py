# Paste into Jupyter if using IDE: %matplotlib auto

# %% Imports
import os
import numpy as np
import random as rand
import matplotlib.pyplot as plt
from scipy import interpolate

import ipywidgets as ipyw
from IPython.display import display, clear_output

# %% Load vars (expensive) - MATLAB v7.3+ (H5PY)
import h5py
loadpath = "20210901-eyescan/Raster2_100mV_cycles_10_phase_0.mat"
with h5py.File(loadpath, "r") as mat:
    time_traces = mat["time_traces"][()].squeeze()
    DP = mat["DP"][()].squeeze()
    UD = mat["UD"][()].squeeze()
    
    print(f"Loaded '{loadpath}' file of shape {time_traces.shape}")

# %% Preprocess signal

tt = time_traces
#tt = np.mean(time_traces, axis=0) # Take mean across 10 samples

# Scope               | (N, UD (y), DP (x)) | MIN_T | MAX_T
# --------------------+---------------------+-------+------
# 1) Unfocused  5 MHz | (20000, 31, 31)     | 15000 | 17000
# 2) Focused   15 MHz | (20000, 31, 31)     |  9000 | 11000
# 3) Unfocused 10 MHz | (40000, 81, 81)     |  2000 |  5000

# Dimensions: (20000, 31, 31) or (40000, 81, 81)
# - 20000 or 40000: time point
# - 31 or 81: up-down (UD)
# - 31 or 81: distal-proximal (DP)

# Waveform properties
fs = 100 # sampling frequency [MHz]
dt = 1/fs
N = tt.shape[0]
T = dt*N

MIN_T  = 1500
MAX_T  = 5500
MIN_DP = 0
MIN_UD = 0
MAX_DP = 81
MAX_UD = 81
MAX_A  = 20 # maximum signal amplitude to scan for -> normalises Bscan

# Move signal on every 2nd DP element by 130 elements to the left
#tt[MIN_T:MAX_T, :, ::2] = tt[MIN_T+130:MAX_T+130, :, ::2]

# Disregard unneeded samples and planes
signal = tt[MIN_T:MAX_T, MIN_UD:MAX_UD, MIN_DP:MAX_DP]
res = signal.shape[0]
tspan = np.linspace(MIN_T, MAX_T, res)*T/N # [us]
zspan = tspan.copy()*1550/1000/2 # [mm]

# %% Demodulator functions
# Demodulator functions

def find_peaks(signal):
    peak_idxs = []

    m_ = 0 # previous derivative
    for x, y in enumerate(signal):
        x_ = max(x-1, 0) # previous x, bounded by 0
        y_ = signal[x_] # previous y

        # Detect peaks by looking at change in local derivative
        m = 0 if x == 0 else (y-y_) / (x-x_) # derivative
        if m_ >= 0 and m < 0:
            peak_idxs.append(x_)
        
        m_ = m # update new derivative
    
    peak_idxs.append(len(signal)-1) # append final index to finish waveform
    
    return np.array(peak_idxs)

def downsample(array, npts):
    array_ds = np.zeros(npts)
    size = len(array)
    for i in range(npts-1):
        idx = int(i * (size-1) / (npts-1))
        p = int(i * (size-1) % (npts-1))

        array_ds[i] = ((p * array[idx+1]) + ((npts-1 - p) * array[idx])) / (npts-1)
    
    array_ds[npts-1] = array[size-1] # done outside of loop to avoid out of bound access
    return array_ds

def process_A(signal, height=None):
    if height is None: height = signal.shape[1] # square by default
        
    sspan = signal.copy()
    sspan[sspan<0] = 0
    
    # Create some vertical limitations
    sspan[sspan>MAX_A] = MAX_A

    # normalise
    #sspan /= np.max(signal)

    # create envelope; demodulate
    peaks_idx = find_peaks(sspan) # extract indices at peaks
    peaks = tspan[peaks_idx] # convert peak index to time unit
    envelope = sspan[peaks_idx] # obtain signal intensity at peak time

    # Downsample into display size
    # Downsampling currently is done on natural envelope instead of smooth to preserve accuracy.
    peaks_ds = downsample(peaks, height)
    envelope_ds = downsample(envelope, height)
    
    # TODO: Inefficient: Just make downsampling uniform in the first place!
    f = interpolate.interp1d(peaks_ds, envelope_ds, fill_value="extrapolate") # uniformly spread out values
    env = f(np.linspace(tspan[0], tspan[-1], height))

    return np.round(env, 4) # round brightness to 4 decimals

def process_B(signal):
    width = signal.shape[1] # lateral resolution: number of pixels in row
    height = width*20 # longitudinal resolution: number of pixels in column
    
    image_565 = np.zeros((width, height)) # rgb 565 format
    
    for col in range(width):
        # Append single column of a B-mode image
        image_565[col, :] = process_A(signal[:, col], height=height)
        
    return image_565

# %% Plotter

def plot(dp, dp_pos, ud, ud_pos):
    # Process signals before clearing
    img_dp = process_B(signal[:, ud, :]) # DP-var
    img_ud = process_B(signal[:, :, dp]) # UD-var
    
    # Reset view
    clear_output(wait=True)
    
    # A-mode plotter
    fig, axs = plt.subplots(1, 1, figsize=(15, 4))

    signal_1d = signal[:, ud, dp]
    signal_1d_up = signal_1d.copy()
    signal_1d_up[signal_1d_up<0] = 0
    signal_1d_down = signal_1d.copy()
    signal_1d_down[signal_1d_down>=0] = 0

    # Local mins/maxs
    #low = np.min(signal_1d)
    #high = np.max(signal_1d)
    low_dp = np.min(img_dp)
    low_ud = np.min(img_ud)
    high_dp = np.max(img_dp)
    high_ud = np.max(img_ud)

    axs.plot(tspan, signal_1d_up, color="black", linewidth=0.5) # raw signal >= 0
    axs.plot(tspan, signal_1d_down, color="black", alpha=0.2, linewidth=0.5) # raw signal < 0
    axs.plot(np.linspace(tspan[0], tspan[-1], img_dp.shape[1]), img_dp[dp], color="darkred")

    # Set the same ylims to absolute limits for easier comparison
    axs.set_ylim([-high_ud/10, high_ud])

    # Set location of iris and pupil
    #axs.axvline(x=154.5, color="orange", alpha=0.3) # Iris 1
    #axs.axvline(x=158, color="red", alpha=0.3) # Pupil 1
    #axs.axvline(x=164.5, color="red", alpha=0.3) # Pupil 2
    #axs.axvline(x=165.5, color="orange", alpha=0.3) # Iris 2
    
    axs.set_xlabel("$z$-position: Time [us]")
    axs.set_ylabel("Signal [mV]")
    axs.set_title(f"x = {dp_pos} mm (plane #{dp+1}), y = {ud_pos} mm (plane #{ud+1})")
    
    # B-mode plotter
    fig, axs = plt.subplots(nrows=1, ncols=2, figsize=(15, 10))
    fig.suptitle("Unfocused B-Scan ($f$ = 10 MHz, $a$ = 0.15 mm)", fontsize=34, y=0.9)
    
    # B-scan 1: DP
    axs[0].imshow(img_dp.T, origin="lower", cmap="binary_r",
                  interpolation="none", aspect=1/2,
                  vmin=0, vmax=high_ud,
                  extent=[DP.min(), DP.max(), zspan.min(), zspan.max()])
    axs[0].text(x=0.025, y=0.025, s="Distal", color="w", fontsize=28, transform=axs[0].transAxes) # distal
    axs[0].text(x=0.69, y=0.025, s="Proximal", color="w", fontsize=28, transform=axs[0].transAxes) # proximal
    axs[0].set_title(f"$y = {ud_pos}$ mm (UD plane #{ud+1})", fontsize=26)
    axs[0].set_xlabel("Relative $x$ [mm]", fontsize=26)
    axs[0].set_ylabel("$z$-position [mm]", fontsize=26)
    axs[0].tick_params(axis='both', which='major', labelsize=22)
    
    # B-scan 2: UD
    axs[1].imshow(np.fliplr(img_ud.T), origin="lower", cmap="binary_r",
                  interpolation="none", aspect=1/2,
                  vmin=0, vmax=high_dp,
                  extent=[UD.min(), UD.max(), zspan.min(), zspan.max()])
    axs[1].text(x=0.025, y=0.025, s="Downward", color="w", fontsize=28, transform=axs[1].transAxes) # upward
    axs[1].text(x=0.71, y=0.025, s="Upward", color="w", fontsize=28, transform=axs[1].transAxes) # downward
    axs[1].set_title(f"$x = {dp_pos}$ mm (DP plane #{dp+1})", fontsize=26)
    axs[1].set_xlabel("Relative $y$ [mm]", fontsize=26)
    axs[1].tick_params(axis='both', which='major', labelsize=22)
    # Do not show y axis on second plot
    #axs[1].set_ylabel("$z$-position [mm]", fontsize=26)
    axs[1].set_yticklabels([])
    
    #fig.tight_layout()
    fig.subplots_adjust(wspace=0.1)
    fig.canvas.flush_events()
    fig.canvas.draw()
    plt.show()# Plot waveforms across both DP and UD

# %% Plot waveforms across both DP and UD

class WaveformRenderer():
    
    def __init__(self, dp_span, ud_span, dp_min_plane=0, ud_min_plane=0):
        self.dp_span = dp_span
        self.ud_span = ud_span
                
        dp_res = round(abs(np.diff(dp_span)[0]), 2)
        ud_res = round(abs(np.diff(ud_span)[0]), 2)
                
        min_dp = dp_span[dp_min_plane]
        min_ud = ud_span[ud_min_plane]
        
        max_dp = min_dp + (signal.shape[2]-1)*dp_res
        max_ud = min_ud + (signal.shape[1]-1)*ud_res
        
        dp_val = (max_dp+min_dp)/2
        ud_val = (max_ud+min_ud)/2
                
        self.out = ipyw.Output(layout=ipyw.Layout(width="1000px", height="900px"))

        self.slider_dp = ipyw.FloatSlider(min=min_dp, max=max_dp, step=dp_res, value=dp_val, readout_format=".1f",
                                          description="DP Plane", layout=ipyw.Layout(width="500px"))
        self.slider_ud = ipyw.FloatSlider(min=min_ud, max=max_ud, step=ud_res, value=ud_val, readout_format=".1f",
                                          description="UD Plane", layout=ipyw.Layout(width="500px"))
        
        widget_dp = ipyw.interactive(self.update_dp, pos=self.slider_dp)
        widget_ud = ipyw.interactive(self.update_ud, pos=self.slider_ud)
        
        self.DP = int(np.where(dp_span == dp_val)[0][0])
        self.UD = int(np.where(ud_span == ud_val)[0][0])
            
    def update_dp(self, pos):
        with self.out:
            self.DP = int(np.where(np.isclose(self.dp_span, pos))[0][0])
            plot(dp=self.DP, dp_pos=self.slider_dp.value, ud=self.UD, ud_pos=self.slider_ud.value)
            
    def update_ud(self, pos):
        with self.out:
            self.UD = int(np.where(np.isclose(self.ud_span, pos))[0][0])
            plot(dp=self.DP, dp_pos=self.slider_dp.value, ud=self.UD, ud_pos=self.slider_ud.value)

    def create(self):
        return ipyw.VBox(children=(self.slider_dp, self.slider_ud, self.out), layout=ipyw.Layout())

view = WaveformRenderer(dp_span=DP, ud_span=UD, dp_min_plane=MIN_DP, ud_min_plane=MIN_UD)
obj = view.create()
display(obj)

# %% End