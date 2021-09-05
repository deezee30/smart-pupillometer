#pragma once

#include <Arduino.h>
#include <Streaming.h>
#include "util/Array.h"
#include "util/timer.hpp"
#include "display/screen.hpp"
#include "serial_server.hpp"

namespace gen {
    static const float ECHO_POS[] = {0, 1.3, 3.2, 3.8, 5, 6, 7}; // microseconds
    static const Array<float, 7> ECHOES(ECHO_POS);
    static const uint16_t RES = 200; // number of points to generate for waveform initially
    static const uint16_t TLIM = 8;
}

template <size_t N>
Array<float, N> linspace(const float min,
                         const float max,
                         const uint16_t n) {
    Array<float, N> result;

    const float step = (max-min) / (floor((float) n) - 1.); // step size
    for (uint16_t i = 0; i < n-1; i++)
        result.push_back(min + i*step); // fill vector

    result.push_back(max); // fix last entry to max
    return result;
}

// TODO: Optimise class
class SignalGenerator {

   public:
    
    static Array<float, gen::RES> generateEcho(const Array<float, gen::RES> tspan,
                                               const uint16_t t) {
        const uint16_t tstart = int(t*gen::RES/gen::TLIM);
        const uint8_t k = 0.2;  // min signal
        Array<float, gen::RES> echo;

        for (uint16_t i = 0; i < gen::RES; i++) {
            if (i < tstart) {
                echo.push_back(0); // delay waveform start
                continue;
            }

            const float A = random(2, 6) / 10.; // amplitude (random)
            float cyc = sin(40*tspan[i]); // sinusoidal component
            float cyc_decay = A * sin((tspan[i]-t-0.5)/0.115)/(tspan[i]-t-0.5) + k; // sinusoidal decay component
            float exp_decay = exp(3*(A-tspan[i]) + 3*t) + k; // exponential decay component

            echo.push_back(abs(cyc * cyc_decay * exp_decay)); // compile waveform
        }

        return echo;
    }

    static Array<float, gen::RES> generateEchoes() {
        float echoes[gen::RES] = {};
        Array<float, gen::RES> tspan = linspace<gen::RES>(0, gen::TLIM, gen::RES);

        // compile randomly generated noisy pulse-echo waveforms
        for (uint8_t t = 0; t < gen::ECHOES.size(); t++) {
            Array<float, gen::RES> echo = SignalGenerator::generateEcho(tspan, gen::ECHOES[t]);
            for (uint16_t i = 0; i < gen::RES; i++)
                echoes[i] += echo[i]/(t+3);
        }

        return Array<float, gen::RES>(echoes);
    }
};

// TODO: Optimise class
class SignalProcessor {

   public:
    static inline float env_max_ = 0;

    template <size_t N>
    static Array<uint16_t, N> findPeaks(const Array<float, N> signal) {
        const uint16_t len = signal.size();
        Array<uint16_t, N> peak_idxs;

        float m_ = 0; // previous derivative
        for (uint16_t x = 0; x < len; x++) {
            const float y = signal[x];
            const uint16_t x_ = max(x-1, 0); // previous x, bounded by 0
            const float y_ = signal[x_]; // previous y

            // Detect local maxima by looking at the change in local derivative
            const float m = x == 0 ? 0 : (y-y_) / float(x-x_); // derivative
            if (m_ >= 0 && m < 0) {
                // Change occurred post-peak; record previous point as a local max
                peak_idxs.push_back(x_);
            }

            m_ = m; // update new derivative
        }

        peak_idxs.push_back(len-1); // append final index to finish waveform
        return peak_idxs;
    }

    // TODO: Make downsampling uniform
    template <size_t N>
    static Array<float, N> downsample(const Array<float, N> array,
                                      const uint16_t npts) {
        const uint16_t oldsize = array.size();
        Array<float, N> array_ds;
        for (uint16_t i = 0; i < npts-1; i++) {
            const uint16_t idx = i * (oldsize-1)/(npts-1);
            const uint16_t p = i * (oldsize-1)%(npts-1);
            array_ds.push_back(((p * array[idx+1]) + ((npts-1 - p) * array[idx])) / (npts-1));
        }
        array_ds.push_back(array[oldsize-1]); // done outside of loop to avoid out of bound access
        return array_ds;
    }

    template <size_t N>
    static Array<float, N> interpLin(const Array<float, N> x,
                                     const Array<float, N> y,
                                     const Array<float, N> x_new,
                                     const bool extrap_bounds=true) {
        const uint16_t n_val = x_new.size(); // resampling number
        Array<float, N> y_new; // interpolated y series
        for (uint16_t idx = 0; idx < n_val; idx++) {
            const float x_pt = x_new[idx];

            // extrapolate when out of bounds
            if (extrap_bounds) {
                if (x_pt <= x[0]) {
                    y_new.push_back(y[0]);
                    continue;
                }
                if (x_pt >= x[n_val-1]) {
                    y_new.push_back(y[n_val-1]);
                    continue;
                }
            }

            auto i = 0;
            float rst = 0;
            if (x_pt <= x[0]) {
                i = 0;
                auto t = (x_pt - x[i]) / (x[i+1] - x[i]);
                rst = y[i]*(1-t) + y[i+1]*t;
            } else if (x_pt >= x[n_val-1]) {
                auto t = (x_pt - x[n_val-2]) / (x[n_val-1] - x[n_val-2]);
                rst = y[n_val-2]*(1-t) + y[n_val-1]*t;
            } else {
                while (x_pt >= x[i+1]) i++;
                auto t = (x_pt - x[i]) / (x[i+1] - x[i]);
                rst = y[i]*(1-t) + y[i+1]*t;
            }

            y_new.push_back(rst);
        }
        return y_new;
    }

    // TODO: Optimise
    static Column receiveAScan(const uint16_t n_rows = IMG_HEIGHT,
                               SerialStream* usb = NULL) {
        // Record time
        Timer timer;
        timer.start();

        const uint16_t RES = cfg::def::MAX_T-cfg::def::MIN_T; // initial axial resolution
        uint16_t init_res = 0;
        float tlim = 1.;
        float min = 0;
        float max = 1;
        Array<float, RES> signal;

        if (usb != NULL && usb->s2()) {
            // If native USB connected, extract echo from stream
            Array<float, RES> echo = usb->listen<RES>();
            for (uint16_t i = 0; i < echo.size(); i++)
                signal.push_back(echo[i]);
            tlim = cfg::acqTime();
            init_res = cfg::numPtsLocal();
            min = tlim;
            max = tlim/(init_res/200);
        } else {
            // Otherwise generate randomly
            Array<float, gen::RES> gen = SignalGenerator::generateEchoes();
            for (uint16_t i = 0; i < min(RES, gen::RES); i++)
                signal.push_back(gen[i]);
            // we are rendering full window of the generated waveform
            tlim = gen::TLIM;
            init_res = gen::RES;
        }

        for (uint16_t i = 0; i < signal.size(); i++) {
            // Disregard all negatives from processing
            if (signal[i] < 0) signal[i] = 0;

            // Create some vertical limitations
            if (signal[i] > 0xFF-cfg::gain()) signal[i] = 0xFF-cfg::gain();
        }
        
        // Create envelope; demodulate
        const Array<uint16_t, RES> peaks_idx = findPeaks(signal); // extract indices at peaks
        const uint16_t n_peaks = peaks_idx.size(); // number of peaks detected

        Array<float, RES> tspan = linspace<RES>(0, tlim, init_res);
        Array<float, RES> peaks; // x axis: time
        Array<float, RES> envelope; // y axis: signal intensity
        for (uint16_t i = 0; i < n_peaks; i++) {
            peaks.push_back(tspan[peaks_idx[i]]); // convert peak index to time unit
            envelope.push_back(signal[peaks_idx[i]]); // obtain signal intensity at peak time
        }

        // Downsample into display size. Currently performed
        // on natural instead of smooth to preserve accuracy.
        Array<float, IMG_HEIGHT> peaks_ds = downsample(peaks, n_rows);
        Array<float, IMG_HEIGHT> env_ds = downsample(envelope, n_rows);

        // Interpolate across series to uniformly spread out the values
        // TODO: May want to make downsampling uniform in the first place
        Array<float, IMG_HEIGHT> env = interpLin(peaks_ds, env_ds, 
                linspace<IMG_HEIGHT>(min, max*tspan[init_res-1], n_rows));
        
        /*
        usb->display_->fillRect(0, 0, 60, 10, usb->display_->colorBlack());
        usb->display_->print(0, 0, sspan.size());
        usb->display_->printFloat(30, 0, sspan[130]);*/

        // Find max val and store across entire BScan
        for (uint16_t i = 1; i < n_rows; i++)
            if (env[i] > env_max_)
                env_max_ = env[i];

        // Convert to RGB565 shade of grey (8-bit) for better storage and faster processing
        Column col = col::BLACK; // greyscale format
        for (uint16_t i = 0; i < n_rows; i++)
            col[i] = round(env[i]*255/env_max_); // normalise between 0 and 255
                
        uint32_t ms = timer.stop();

        return col;
    }

    // TODO: Optimise
    static Image receiveBScan(const uint16_t n_rows = IMG_HEIGHT,
                              const uint16_t n_cols = IMG_WIDTH,
                              SerialStream* usb = NULL) {
        // stored and processed transposed for easy column extraction
        Image image; // greyscale format

        // Record time
        Timer timer;
        timer.start();
        for (uint16_t col = 0; col < n_cols; col++) {
            Column column = receiveAScan(n_rows, usb);

            // Append single column of a B-mode image
            image.push_back(column);
        }
        uint32_t ms = timer.stop();

        return image; // return greyscale format
    }
};