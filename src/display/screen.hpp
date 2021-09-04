#pragma once

#include <Arduino.h>
#include "../util/Array.h"

// Layout sizing
#define TOP_HEIGHT          16
#define SIDE_WIDTH_LEFT     28
#define SIDE_WIDTH_RIGHT    20
// Viewport sizing before rotation
#define SCREEN_HEIGHT       160 // Initial screen height
#define SCREEN_WIDTH        128 // Initial screen width
// Sizes defined after screen rotation
#define IMG_HEIGHT (SCREEN_WIDTH-TOP_HEIGHT)                        // longitudinal height (# rows)
#define IMG_WIDTH  (SCREEN_HEIGHT-SIDE_WIDTH_LEFT-SIDE_WIDTH_RIGHT) // lateral width (# columns)
// Positioning (Left Sidebar)
#define S1_YPOS             100          // Programming USB port (S1) y positioning
#define S2_YPOS             (S1_YPOS+10) // Native USB port (S2) y positioning

using Row       = Array<uint8_t, IMG_WIDTH>;
using Column    = Array<uint8_t, IMG_HEIGHT>;
using Image     = Array<Column,  IMG_WIDTH>;

// Column
namespace col {
    inline Column createUniform(uint8_t shade) {
        Column col;
        for (uint8_t r = 0; r < col.max_size(); r++)
            col[r] = shade;
        return col;
    }

    static const Column BLACK = createUniform(0);   // black (empty) column
    static const Column WHITE = createUniform(255); // white column
}

// Configuration
namespace cfg {
    // Options (Left Sidebar)
    static const uint8_t _DEFAULT_FREQ_      = 10;
    static const uint8_t _DEFAULT_IMG_SCALE_ = 1;
    static const uint8_t _DEFAULT_ACQ_TIME_  = 8;

    inline bool    config_update_ = false; // scheduled update of configuration settings
    inline uint8_t freq_          = _DEFAULT_FREQ_; // ultrasound probe operational frequency [MHz]
    inline uint8_t img_scale_     = _DEFAULT_IMG_SCALE_; // linear amplification of image
    inline uint8_t acq_time_      = _DEFAULT_ACQ_TIME_; // acquisition time [us]

    inline void update(const uint8_t freq,
                       const uint8_t img_scale,
                       const uint8_t acq_time) {
        cfg::freq_ = freq;
        // cfg::img_scale_ = img_scale; do not update img_scale more than
        cfg::acq_time_ = acq_time;

        config_update_ = true;
    }

    inline void finishUpdate()       { config_update_ = false; }
    inline bool scheduledUpdate()    { return config_update_; }
    inline uint8_t frequency()       { return freq_; }
    inline uint8_t imageScale()      { return img_scale_; }
    inline uint8_t acquisitionTime() { return acq_time_; }
}