#pragma once

#include "Arduino.h"
class ColorUtil {
   public:
    uint16_t grayRGB565To16(const uint8_t shade) {
        return ((shade & 0xF8) << 8) | ((shade & 0xFC) << 3) | (shade >> 3);
    }

    uint16_t grayFloatTo16(const float shade) {
        return grayRGB565To16(round(shade*255));
    }

    uint8_t gray16ToRGB565(const uint16_t shade) {
        uint8_t r = (shade >> 8) & 0xF8; r |= (r >> 5);
        uint8_t g = (shade >> 3) & 0xFC; g |= (g >> 6);
        uint8_t b = (shade << 3) & 0xF8; b |= (b >> 5);

        return round((r+g+b)/3);
    }
};