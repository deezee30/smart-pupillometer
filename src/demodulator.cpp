#include <demodulator.h>

static const uint8_t tlim = 8;     // us
static const uint16_t res = 2500;  // points

static const uint16_t height = 112;  // longitudinal resolution: number of pixels in column
static const uint16_t width = 112;   // lateral resolution: number of pixels in row

float *linspace(const float a, const float b, const int n) { 
    float c;
    float e[n];
    
    // step size
    c = (b-a) / (n-1);
    
    // fill vector
    for(uint16_t i = 0; i < n-1; ++i)
        e[i] = a + i*c;
    
    // fix last entry to b
    e[n-1] = b;
    
    return e;
}

uint16_t *findPeaks(const float *signal,
                    const uint16_t len) {
    uint16_t *peak_idxs = {};
    uint16_t peak_count = 0;

    float m_ = 0; // previous derivative
    for (uint16_t x = 0; x < len; x++) {
        const uint16_t y = signal[x];
        const uint16_t x_ = max(x-1, 0); // previous x, bounded by 0
        const uint16_t y_ = signal[x_]; // previous y

        // Detect local maxima by looking at the change in local derivative
        const float m = x == 0 ? 0 : (y-y_) / (x-x_); // derivative
        if (m_ >= 0 && m < 0) {
            // Change occurred post-peak; record previous point as a local max
            peak_idxs[peak_count++] = x_;
        }

        m_ = m; // update new derivative
    }

    peak_idxs[peak_count++] = len-1; // append final index to finish waveform

    return peak_idxs;
}

uint16_t gray_rgb565to16bit(const uint8_t shade) {
    return ((shade & 0xF8) << 8) | ((shade & 0xFC) << 3) | (shade >> 3);
}

uint8_t gray_16bittorgb565(const uint16_t shade) {
    uint8_t r = (shade >> 8) & 0xF8; r |= (r >> 5);
    uint8_t g = (shade >> 3) & 0xFC; g |= (g >> 6);
    uint8_t b = (shade << 3) & 0xF8; b |= (b >> 5);

    return round((r+g+b)/3);
}

float *generateEcho(const float *tspan, const uint16_t t,
                    const uint8_t tlim, const uint16_t res) {
    const uint16_t tstart = int(t*res/tlim);
    const uint8_t k = 0.2;  // min signal
    float *echo = {};

    for (uint16_t i = 0; i < res; i++) {
        if (i < tstart) {
            echo[i] = 0;
            continue;
        }

        const float A = random(2, 6) / 10.; // amplitude (random)

        float cyc = sin(40*tspan[i]); // sinusoidal component
        float cyc_decay = A * sin((tspan[i]-t-0.5)/0.115)/(tspan[i]-t-0.5) + k; // sinusoidal decay component
        float exp_decay = exp(3*(A-tspan[i]) + 3*t) + k; // exponential decay component

        echo[i] = abs(cyc * cyc_decay * exp_decay); // compile waveform
    }

    return echo;
}

template <typename T, typename U>
T *downsample(const U *array,
              const uint16_t oldsize,
              const uint16_t npts) {
    float array_ds[npts] = {};
    for (uint16_t i = 0; i < npts-1; i++) {
        const uint16_t idx = i * (oldsize-1)/(npts-1);
        const uint16_t p = i * (oldsize-1)%(npts-1);
        array_ds[i] = ((p * array[idx+1]) + ((npts-1 - p) * array[idx])) / (npts-1);
    }
    array_ds[npts-1] = array[oldsize-1]; // done outside of loop to avoid out of bound access
    return array_ds;
}

uint16_t **generateBScan(const float *echos, const uint8_t peaks) {

    static const float *tspan = linspace(0, tlim, res); // time span [us]

    // stored and processed transposed for easy column extraction
    //double **image = {}; // float format
    uint16_t **image_565 = {}; // rgb565 format

    for (uint16_t col = 0; col < width; col++) {
        float sspan[res] = {};

        // Compile noisy pulse-echo waveform
        for (uint8_t tx = 0; tx < peaks; tx++) {
            const float t = echos[tx];
            const float *echo = generateEcho(tspan, t, tlim, res);

            for (uint16_t i = 0; i < res; i++)
                sspan[i] += echo[i]/(tx+3);
        }

        // Find max val
        float max = sspan[0];
        for (uint16_t i = 1; i < res; i++)
            if (sspan[i] > max)
                max = sspan[i];

        // Normalise
        for (uint16_t i = 0; i < res; i++)
            sspan[i] /= max;
        
        // Create envelope; demodulate
        const uint16_t *peaks_idx = findPeaks(sspan, res); // extract indexes at peaks
        const uint16_t n_peaks = sizeof(peaks_idx) / sizeof(peaks_idx[0]);
        uint16_t peaks[n_peaks] = {}; // x axis: time
        float envelope[n_peaks] = {}; // y axis: signal intensity
        for (uint16_t i = 0; i < n_peaks; i++) {
            peaks[i] = peaks_idx[i] * tlim/res; // convert peak index to time unit
            envelope[i] = sspan[peaks_idx[i]]; // obtain signal intensity at peak time
        }

        // TODO: Upsample and smoothen

        // Downsample into display size
        // Downsampling currently is done on natural envelope instead of smooth to preserve accuracy.
        //const uint16_t *peaks_ds = downsample<uint16_t>(peaks, n_peaks, height);
        const float *envelope_ds = downsample<float>(envelope, n_peaks, height);

        // TODO: Display the first scan pulse-echo

        // Append single column of a B-mode image
        for (uint16_t i = 0; i < height; i++) {
            //image[col][i] = envelope_ds[i];
            // Convert pixels to RGR565 shades of gray
            image_565[col][i] = gray_rgb565to16bit(round(envelope_ds[i]*255));
        }
    }

    // TODO: Show (transposed) image

    // return rgb565 format
    return image_565;
}