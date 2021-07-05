#pragma once

// Includes
#include <color_util.h>

#define IMG_WIDTH           112 // Ultrasound image width (after rotation)
#define IMG_HEIGHT          112 // Ultrasound image height (after rotation)
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

    bool setup(const uint16_t image[IMG_WIDTH][IMG_HEIGHT] = NULL);

    // uint16_t processing is faster than float
    bool setup(const float image[IMG_WIDTH][IMG_HEIGHT]) {
        uint16_t scan_16[IMG_WIDTH][IMG_HEIGHT] = {};

        for (uint16_t c = 0; c < IMG_WIDTH; c++)
            for (uint16_t r = 0; r < IMG_HEIGHT; r++)
                scan_16[c][r] = grayFloatTo16(image[c][r]);

        return setup(scan_16);
    }

    bool renderTitle();

    bool renderLeft();

    bool renderRight();

    bool renderInner(const uint16_t scan[IMG_WIDTH][IMG_HEIGHT],
                     const uint8_t scale = IMG_MAG);

    // float processing is slower than uint16_t
    bool renderInner(const float scan[IMG_WIDTH][IMG_HEIGHT],
                     const uint8_t scale = IMG_MAG) {
        uint16_t scan_16[IMG_WIDTH][IMG_HEIGHT] = {};

        for (uint16_t c = 0; c < IMG_WIDTH; c++)
            for (uint16_t r = 0; r < IMG_HEIGHT; r++)
                scan_16[c][r] = grayFloatTo16(scan[c][r]);

        return renderInner(scan_16, scale);
    }

    bool renderColumn(uint16_t col,
                      const uint16_t scan[IMG_HEIGHT],
                      const uint8_t scale = IMG_MAG);
    
    // float processing is slower than uint16_t
    bool renderColumn(uint16_t col,
                      const float scan[IMG_HEIGHT],
                      const uint8_t scale = IMG_MAG) {
        uint16_t scan_16[IMG_HEIGHT] = {};

        for (uint16_t r = 0; r < IMG_HEIGHT; r++)
            scan_16[r] = grayFloatTo16(scan[r]);

        return renderColumn(col, scan_16, scale);
    }

    bool renderFull(const uint16_t inner[IMG_WIDTH][IMG_HEIGHT] = NULL,
                    const uint8_t scale = IMG_MAG) {
        return renderTitle()                 // Draw heading: Title
               & renderLeft()                // Draw left sidebar: Options
               & renderRight()               // Draw right sidebar: Scale
               & renderInner(inner, scale);  // Refresh inner view
    }

    // float processing is slower than uint16_t
    bool renderFull(const float inner[IMG_WIDTH][IMG_HEIGHT] = NULL,
                    const uint8_t scale = IMG_MAG) {
        return renderTitle()                 // Draw heading: Title
               & renderLeft()                // Draw left sidebar: Options
               & renderRight()               // Draw right sidebar: Scale
               & renderInner(inner, scale);  // Refresh inner view
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