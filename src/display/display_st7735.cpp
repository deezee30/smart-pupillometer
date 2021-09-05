#include "display_st7735.hpp"

uint16_t DisplayST7735::getHeight()                         { return tft.height(); }
uint16_t DisplayST7735::getWidth()                          { return tft.width(); }
void DisplayST7735::init()                                  { tft.init(); }
void DisplayST7735::setRotation(uint8_t rot)                { tft.setRotation(rot); }
void DisplayST7735::fillScreen(uint16_t c)                  { tft.fillScreen(c); }
void DisplayST7735::setTextSize(uint8_t size)               { tft.setTextSize(size); }
void DisplayST7735::setTextFont(uint8_t font)               { tft.setTextFont(font); }
void DisplayST7735::fillRect(uint16_t x, uint16_t y, uint16_t w,
                             uint16_t h, uint16_t c)        { tft.fillRect(x, y, w, h, c); }
void DisplayST7735::setCursor(uint16_t x, uint16_t y,
                              uint8_t font)                 { tft.setCursor(x, y, font); }
void DisplayST7735::setTextColor(uint16_t c)                { tft.setTextColor(c); }
void DisplayST7735::drawFastVLine(uint32_t x, uint32_t y,
                                  uint32_t h, uint32_t c)   { tft.drawFastVLine(x, y, h, c); }
void DisplayST7735::drawFastHLine(uint32_t x, uint32_t y,
                                  uint32_t w, uint32_t c)   { tft.drawFastHLine(x, y, w, c); }
void DisplayST7735::drawPixel(uint32_t x, uint32_t y,
                              uint32_t c)                   { tft.drawPixel(x, y, c); }

uint16_t DisplayST7735::colorRed()          { return TFT_RED; }
uint16_t DisplayST7735::colorGreen()        { return TFT_GREEN; }
uint16_t DisplayST7735::colorBlack()        { return TFT_BLACK; }
uint16_t DisplayST7735::colorWhite()        { return TFT_WHITE; }
uint16_t DisplayST7735::colorLightGrey()    { return TFT_LIGHTGREY; }
uint16_t DisplayST7735::colorDarkGrey()     { return TFT_DARKGREY; }
uint16_t DisplayST7735::colorScale()        { return SCALE_COLOR; }

uint8_t DisplayST7735::fontTitle()          { return 2; }
uint8_t DisplayST7735::fontContent()        { return 1; }

Print* DisplayST7735::out() { return &tft; }

// Print delegation to libraries often breaks, therefore naturally draw the content instead
int16_t DisplayST7735::print(int32_t poX, int32_t poY, const __FlashStringHelper* content) {
    return print(poX, poY, reinterpret_cast<const char *>(content));
}
int16_t DisplayST7735::print(int32_t poX, int32_t poY, const String& content) {
    return tft.drawString(content, poX, poY);
}
int16_t DisplayST7735::print(int32_t poX, int32_t poY, const char content[]) {
    return tft.drawString(content, poX, poY);
}
int16_t DisplayST7735::print(int32_t poX, int32_t poY, long content) {
    return tft.drawNumber(content, poX, poY);
}
int16_t DisplayST7735::printFloat(int32_t poX, int32_t poY, float f, uint8_t dp) {
    return tft.drawFloat(f, dp, poX, poY);
}