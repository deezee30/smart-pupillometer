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

#define DEFAULT_IMAGE_SCALE 1

using Row       = Array<uint8_t, IMG_WIDTH>;
using Column    = Array<uint8_t, IMG_HEIGHT>;
using Image     = Array<Column,  IMG_WIDTH>;