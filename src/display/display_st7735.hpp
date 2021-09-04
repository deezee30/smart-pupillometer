#pragma once

#include <TFT_eSPI.h>
#include "display.hpp"

#define TFT_HEIGHT  SCREEN_HEIGHT
#define TFT_WIDTH   SCREEN_WIDTH
#define SCALE_COLOR TFT_GOLD

class DisplayST7735 : public Display {

   public:
    TFT_eSPI tft;

    DisplayST7735(const uint8_t image_scale = cfg::imageScale()) :
        Display(image_scale),
        tft(TFT_eSPI(TFT_WIDTH, TFT_HEIGHT)) { // invoke library; pins defined in User_Setup.h
    }

    ~DisplayST7735() {

    }

    virtual uint16_t getHeight(),
                     getWidth() override;

    virtual void init(),
                 setRotation(uint8_t rot),
                 fillScreen(uint16_t c),
                 setTextSize(uint8_t size),
                 setTextFont(uint8_t font),
                 fillRect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t c),
                 setCursor(uint16_t x, uint16_t y, uint8_t font),
                 setTextColor(uint16_t c),
                 drawFastVLine(uint32_t x, uint32_t y, uint32_t h, uint32_t c),
                 drawFastHLine(uint32_t x, uint32_t y, uint32_t w, uint32_t c),
                 drawPixel(uint32_t x, uint32_t y, uint32_t c) override;

    virtual uint16_t colorRed(),
                     colorGreen(),
                     colorBlack(),
                     colorWhite(),
                     colorLightGrey(),
                     colorDarkGrey(),
                     colorScale() override;
    virtual uint8_t  fontTitle(),
                     fontContent() override;

    virtual Print* out() override;

    virtual int16_t print(int32_t, int32_t, const __FlashStringHelper*),
                    print(int32_t, int32_t, const String&),
                    print(int32_t, int32_t, const char[]),
                    print(int32_t, int32_t, long) override;
};