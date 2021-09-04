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
// TODO: Move to separate file
namespace cfg {
    // Options (Left Sidebar)
    static const uint8_t CFG_FREQUENCY      = 10;
    static const uint8_t CFG_IMG_SCALE      = 1;
    static const uint8_t CFG_ACQ_TIME       = 8;
    static const uint8_t CFG_SAMP_RATE      = 100;
    static const uint8_t CFG_NUM_PTS_GLOBAL = 40000;
    static const uint8_t CFG_NUM_PTS_LOCAL  = 3000;
    static const uint8_t CFG_MIN_T          = 1500;
    static const uint8_t CFG_MAX_T          = 5500;
    static const uint8_t CFG_MAX_A          = 20;
    static const uint8_t CFG_SPEED_SOUND    = 1550;
    static const uint8_t CFG_BSCAN_DP       = true;
    static const uint8_t CFG_SELECT_PLANE   = 40;

    inline bool     config_update_ = false; // scheduled update of configuration settings
    inline uint8_t  freq           = CFG_FREQUENCY; // ultrasound probe operational frequency [MHz]
    inline uint8_t  img_scale      = CFG_IMG_SCALE; // linear amplification of image
    inline uint8_t  acq_time       = CFG_ACQ_TIME; // acquisition time [us]
    inline uint16_t samp_rate      = CFG_SAMP_RATE;
    inline uint32_t num_pts_global = CFG_NUM_PTS_GLOBAL;
    inline uint16_t num_pts_local  = CFG_NUM_PTS_LOCAL;
    inline uint16_t min_t          = CFG_MIN_T;
    inline uint16_t max_t          = CFG_MAX_T;
    inline uint8_t  max_a          = CFG_MAX_A;
    inline uint16_t speed_sound    = CFG_SPEED_SOUND;
    inline bool     bscan_dp       = CFG_BSCAN_DP;
    inline uint8_t select_plane    = CFG_SELECT_PLANE;

    inline void update(const uint8_t freq,           const uint8_t img_scale,
                       const uint8_t acq_time,       const uint8_t samp_rate,
                       const uint8_t num_pts_global, const uint8_t num_pts_local,
                       const uint8_t min_t,          const uint8_t max_t,
                       const uint8_t max_a,          const uint8_t speed_sound,
                       const uint8_t bscan_dp,       const uint8_t select_plane) {
        cfg::freq = freq;
        // cfg::img_scale = img_scale; do not update img_scale more than
        cfg::acq_time = acq_time;
        cfg::samp_rate = samp_rate;
        cfg::num_pts_global = num_pts_global;
        cfg::num_pts_local = num_pts_local;
        cfg::min_t = min_t;
        cfg::max_t = max_t;
        cfg::max_a = max_a;
        cfg::speed_sound = speed_sound;
        cfg::bscan_dp = bscan_dp;
        cfg::select_plane = select_plane;

        config_update_ = true;
    }

    inline void finishUpdate()       { config_update_ = false; }
    inline bool scheduledUpdate()    { return config_update_; }
}