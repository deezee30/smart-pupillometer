#pragma once

#include <Arduino.h>
#include <Array.h>

class Demodulator {

   public:
    static float *linspace(const float, const float, const int);

    static uint16_t *findPeaks(const float *, const uint16_t);

    static uint16_t gray_rgb565to16bit(const uint8_t);

    static uint8_t gray_16bittorgb565(const uint16_t);

    static float *generateEcho(const float *, const uint16_t, const uint8_t, const uint16_t);

    template <typename T, typename U>
    static T *downsample(const U *, const uint16_t, const uint16_t);

    static uint16_t **generateBScan(const float *, const uint8_t);
};