import matplotlib.pyplot as plt
import numpy as np
import random as rand
from scipy.signal import find_peaks, savgol_filter
from scipy import interpolate

def gen_echo(t, h, tlim):
    res = len(t)
    tstart = int(h*res/tlim)

    k = 0.2  # min signal
    A = rand.uniform(0.2, 0.6) # amplitude (random)
    A = 0.4
    sin = np.sin(40*t)  # sinusoidal component
    sin_decay = A*np.sin((t-h-0.5)/0.115)/(t-h-0.5) + k # sinusoidal decay component
    exp_decay = np.exp(3*(A-t)+3*h) + k  # exponential decay component
    echo = sin*sin_decay*exp_decay
    echo[:tstart] = 0

    return np.abs(echo)

def find_peaks_natural(signal):
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
    interpolated = interpolate.interp1d(np.arange(len(array)), array, fill_value="extrapolate")
    downsampled = interpolated(np.linspace(0, len(array), npts))
    return downsampled

def downsample_natural(array, npts):
    array_ds = np.zeros(npts)
    size = len(array)
    for i in range(npts-1):
        idx = int(i * (size-1) / (npts-1))
        p = int(i * (size-1) % (npts-1))

        array_ds[i] = ((p * array[idx+1]) + ((npts-1 - p) * array[idx])) / (npts-1)
    
    array_ds[npts-1] = array[size-1] # done outside of loop to avoid out of bound access
    return array_ds

def gray_rgb565to16bit(shade):
    return ((shade & 0xF8) << 8) | ((shade & 0xFC) << 3) | (shade >> 3)

def gray_16bittorgb565(shade):
    r = (shade >> 8) & 0xF8; r |= (r >> 5)
    g = (shade >> 3) & 0xFC; g |= (g >> 6)
    b = (shade << 3) & 0xF8; b |= (b >> 5)

    return round((r+g+b)/3)

def main():
    tlim = 8  # us
    res = 250  # points

    tspan = np.linspace(0, tlim, res)  # time span [us]

    height = 112 # longitudinal resolution: number of pixels in column
    width = 112 # lateral resolution: number of pixels in row

    echos = [0, 1.3, 3.2, 3.8, 5, 6, 7] # microseconds

    image = np.zeros((width, height)) # stored and processed transposed for easy column extraction

    for c in range(width):
        sspan = np.zeros(res)

        for x, t in enumerate(echos):
            sspan += gen_echo(tspan, t, tlim)/(x+3)
        
        # normalise
        sspan /= max(sspan)

        # create envelope / demodulate
        # peaks_idx, _ = find_peaks(sspan, width=res/500) # using SciPy
        peaks_idx = find_peaks_natural(sspan) # natural
        peaks = peaks_idx*tlim/res
        signal = sspan[peaks_idx]

        # Upsample and smoothen
        signal_sm = signal
        f = interpolate.interp1d(peaks, signal, kind="cubic", fill_value="extrapolate")
        peaks_sm = np.linspace(0, tlim, res)
        signal_sm = f(peaks_sm)

        # Downsample into display size
        # Downsampling currently is done on natural enveloped instead of smooth to preserve accuracy.
        peaks_ds = downsample_natural(peaks, height)
        signal_ds = downsample_natural(signal, height)

        # Display the first scan pulse-echo
        if c == 0:
            fig, ax = plt.subplots(nrows=1, ncols=1)
            
            # Demodulation
            ax.plot(tspan, sspan, label="signal")
            ax.plot(peaks, signal, label="envelope")
            #ax.plot(peaks_sm, signal_sm, label="smooth envelope")
            ax.plot(peaks_ds, signal_ds, label="downsampled envelope")
            ax.legend()
            ax.set_title("A-Mode Pulse-Echo for scan #1")
            ax.set_xlabel("Echo time (us)")
            ax.set_ylabel("Signal intensity")

        # Append single column of a B-mode image
        image[c, :] = np.round(signal_ds, 4) # round brightness to 4 decimals

    # Show image
    plt.figure()
    plt.clf()
    plt.imshow(image.T, origin="lower", cmap="binary_r", interpolation="none")
    plt.title("B-Mode image")
    plt.show()

    # Convert pixels to RGR565 shades of gray
    image_gr = np.zeros(image.shape, dtype=np.uint16)
    for c, r in np.ndindex(image_gr.shape):
        image_gr[c, r] = round(image[c, r]*255)

    print(str(image_gr.tolist()).replace("[", "{").replace("]", "}"))

if __name__ == "__main__": main()