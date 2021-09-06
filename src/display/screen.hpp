#pragma once

#include <Arduino.h>
#include "../util/Array.h"
#include "../config.hpp"

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
#define S_YPOS              120 // S1 and S2 vertical positioning

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