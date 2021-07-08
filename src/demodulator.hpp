#pragma once

#include <Arduino.h>
#include <Streaming.h>
#include "util/Array.h"
#include "util/timer.hpp"
#include "color_util.h"

#define IMG_WIDTH  112 // width after rotation - lateral resolution (# pixels in a row)
#define IMG_HEIGHT 112 // height after rotation - longitudinal resolution (# pixels in column)

using Row = Array<uint8_t, IMG_HEIGHT>;
using Column = Array<uint8_t, IMG_WIDTH>;
using Image = Array<Row, IMG_WIDTH>;

static const uint8_t tlim = 8;     // us
static const uint16_t res = 250;  // points

// TODO: Optimise class
class Demodulator {

   public:
    template <size_t N>
    static Array<float, N> linspace(float min, float max) {
        Array<float, N> result;

        const float step = (max-min) / (floor((float) N) - 1.); // step size
        for (uint16_t i = 0; i < N-1; i++)
            result.push_back(min + i*step); // fill vector

        result.push_back(max); // fix last entry to max
        return result;
    }

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

    // Friday:
    // TODO: Also, email Kozlov about cow eye, progress and component form. Update component list.
    // TODO: Respond to Shusei email and fill out the form.
    // TODO: Next steps with ultrasound probe
    // TODO: Overleaf + introduction
    // TODO: Natwest
    template <size_t N>
    static Array<float, N> generateEcho(const Array<float, N> tspan,
                                        const uint16_t t,
                                        const uint8_t tlim) {
        const uint16_t tstart = int(t*res/tlim);
        const uint8_t k = 0.2;  // min signal
        Array<float, res> echo;

        for (uint16_t i = 0; i < res; i++) {
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

    // TODO: Optimise
    template <size_t N>
    static Column generateAScan(const Array<float, N> echos, uint16_t col) {
        static const Array<float, res> tspan = linspace<res>(0, tlim); // time span [us]

        Column scan_565; // rgb565 format
        float sspan[res] = {};

        // Compile noisy pulse-echo waveform
        for (uint8_t t = 0; t < echos.size(); t++) {
            const Array<float, res> echo = generateEcho(tspan, echos[t], tlim);

            for (uint16_t i = 0; i < res; i++)
                sspan[i] += echo[i]/(t+3);
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
        Array<float, res> sspan_arr(sspan);
        const Array<uint16_t, res> peaks_idx = findPeaks(sspan_arr); // extract indexes at peaks
        const uint16_t n_peaks = peaks_idx.size();
        Array<uint16_t, res> peaks; // x axis: time
        Array<float, res> envelope; // y axis: signal intensity
        for (uint16_t i = 0; i < n_peaks; i++) {
            peaks.push_back(peaks_idx[i] * tlim/res); // convert peak index to time unit
            envelope.push_back(sspan[peaks_idx[i]]); // obtain signal intensity at peak time
        }

        // TODO: Upsample and smoothen

        // Downsample into display size
        // Downsampling currently is done on natural envelope instead of smooth to preserve accuracy.
        //const Array<uint16_t, res> peaks_ds = downsample<uint16_t>(peaks, IMG_HEIGHT);
        const Array<float, res> envelope_ds = downsample(envelope, IMG_HEIGHT);

        // TODO: Display the first scan pulse-echo

        // Convert to RGB565 shade of grey (8-bit) for better storage and faster processing
        for (uint16_t i = 0; i < IMG_HEIGHT; i++)
            scan_565.push_back(round(envelope_ds[i]*255));

        return scan_565;
    }

    // TODO: Optimise
    template <size_t N>
    static Image generateBScan(const Array<float, N> echos) {
        static const Array<float, res> tspan = linspace<res>(0, tlim); // time span [us]

        // stored and processed transposed for easy column extraction
        Image image_565; // rgb565 format

        // Record time and show progress
        Timer timer;
        timer.start();
        //Serial << '[';

        for (uint16_t col = 0; col < IMG_WIDTH; col++) {
            Column column = generateAScan(echos, col);

            // Append single column of a B-mode image
            image_565.push_back(column);
            //Serial << '.';
        }

        uint32_t ms = timer.stop();
        //Serial << F("] (") << ms << F(" ms)") << endl;

        // TODO: Show (transposed) image

        // return rgb565 format
        return image_565;
    }
};