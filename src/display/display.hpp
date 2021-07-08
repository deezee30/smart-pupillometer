#pragma once

// Includes
#include "../color_util.h"
#include "../demodulator.hpp"

#define IMG_MAG             1   // Linear amplification of image

#define TOP_HEIGHT          16
#define SIDE_WIDTH_LEFT     28
#define SIDE_WIDTH_RIGHT    20

// View

// Scalebar (Right Sidebar)
#define SCALEBAR_PAD        4
#define SCALEBAR_TICKS      6   // amount of markers on the right side scale bar
#define SCALEBAR_RANGE      8   // scale bar range, divisible by (SCALEBAR_TICKS+2) [us]
#define SCALEBAR_TICK_SIZE  3   // length of tick marks for the scale bar

class Display : public ColorUtil {
   protected:
    uint8_t us_freq; // MHz

   public:
    Display(const uint8_t us_freq)
        : us_freq(us_freq) {}

    ~Display() {
        
    }

    bool setup();

    bool setup(const Image scan);

    bool renderTitle();

    bool renderLeft();

    bool renderRight();

    bool renderInner(const Image scan,
                     const uint8_t scale = IMG_MAG);

    bool renderColumn(uint16_t col,
                      const Column scan,
                      const uint8_t scale = IMG_MAG);
    
    bool renderDefault() {
        return renderTitle()    // Draw heading: Title
               & renderLeft()   // Draw left sidebar: Options
               & renderRight(); // Draw right sidebar: Scale
    }

    bool clear();

    bool clearInner();

    virtual uint16_t getHeight() = 0,
                     getWidth() = 0;

    virtual void init() = 0,
                 setRotation(uint8_t rot) = 0,
                 fillScreen(uint16_t c) = 0,
                 setTextSize(uint8_t size) = 0,
                 setTextFont(uint8_t font) = 0,
                 fillRect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t c) = 0,
                 setCursor(uint16_t x, uint16_t y, uint8_t font) = 0,
                 setTextColor(uint16_t c) = 0,
                 drawFastVLine(uint32_t x, uint32_t y, uint32_t h, uint32_t c) = 0,
                 drawFastHLine(uint32_t x, uint32_t y, uint32_t w, uint32_t c) = 0,
                 drawPixel(uint32_t x, uint32_t y, uint32_t c) = 0;
    
    virtual uint16_t colorBlack() = 0,
                     colorWhite() = 0,
                     colorLightGrey() = 0,
                     colorDarkGrey() = 0,
                     colorScale() = 0;
    virtual uint8_t  fontTitle() = 0,
                     fontContent() = 0;
    
    // TODO: Smart pointers
    virtual Print* out() = 0;

    virtual int16_t print(int32_t, int32_t, const __FlashStringHelper*) = 0,
                    print(int32_t, int32_t, const String&) = 0,
                    print(int32_t, int32_t, const char[]) = 0,
                    print(int32_t, int32_t, long) = 0;
};