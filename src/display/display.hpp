#pragma once

// Includes
#include "../color_util.h"
#include "../demodulator.hpp"

// View

// Scalebar (Right Sidebar)
#define SCALEBAR_PAD        4
#define SCALEBAR_TICKS      6   // amount of markers on the right side scale bar
#define SCALEBAR_RANGE      8   // scale bar range, divisible by (SCALEBAR_TICKS+2) [us]
#define SCALEBAR_TICK_SIZE  3   // length of tick marks for the scale bar

class Display : public ColorUtil {
   protected:
    uint8_t us_freq_;
    uint8_t image_scale_;
    uint16_t image_rows_;
    uint16_t image_cols_;

   public:
    Display(const uint8_t us_freq,
            const uint8_t image_scale = DEFAULT_IMAGE_SCALE)
        : us_freq_(us_freq)
        , image_scale_(image_scale)
        , image_rows_(IMG_HEIGHT/image_scale)
        , image_cols_(IMG_WIDTH/image_scale) {}

    ~Display() {
        
    }

    uint16_t getRows()      { return image_rows_; }
    uint16_t getColumns()   { return image_cols_; }
    uint8_t getImageScale() { return image_scale_; }

    bool setup();

    bool setup(const Image scan);

    bool renderTitle();

    bool renderLeft();

    bool renderRight();

    bool renderInner(const Image scan);

    bool renderColumn(uint16_t col,
                      const Column scan);
    
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
    
    virtual uint16_t colorRed() = 0,
                     colorGreen() = 0,
                     colorBlack() = 0,
                     colorWhite() = 0,
                     colorLightGrey() = 0,
                     colorDarkGrey() = 0,
                     colorScale() = 0;
    virtual uint8_t  fontTitle() = 0,
                     fontContent() = 0;
    
    virtual Print* out() = 0;

    virtual int16_t print(int32_t, int32_t, const __FlashStringHelper*) = 0,
                    print(int32_t, int32_t, const String&) = 0,
                    print(int32_t, int32_t, const char[]) = 0,
                    print(int32_t, int32_t, long) = 0;
};