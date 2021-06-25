#include <display.h>
#include <TFT_eSPI.h>

#define TFT_WIDTH 128  // Initial screen width (before rotation)
#define TFT_HEIGHT 160  // Initial screen height (before rotation)

#define IMG_WIDTH 57 // Ultrasound image width (after rotation)
#define IMG_HEIGHT 57 // Ultrasound image height (after rotation)
#define IMG_MAG 2

#define TOP_HEIGHT 18
#define SIDE_WIDTH_LEFT 28
#define SIDE_WIDTH_RIGHT 20

#define TFT_FONT 2

// View
#define SIDEBAR_FONT 1  // font to use for the side bars

// Options (Left Sidebar)
#define US_MHZ 5 // Ultrasound probe operational frequency [MHz]

// Scalebar (Right Sidebar)
#define SCALEBAR_PAD 4
#define SCALEBAR_TICKS 6              // amount of markers on the right side scale bar
#define SCALEBAR_RANGE 8              // scale bar range, divisible by (SCALEBAR_TICKS+2) [us]
#define SCALEBAR_TICK_SIZE 3          // length of tick marks for the scale bar
#define SCALEBAR_COLOUR TFT_DARKGREY  // colour of scale bar

class Display {

    TFT_eSPI tft(TFT_WIDTH, TFT_HEIGHT);  // Invoke library, pins defined in User_Setup.h

    const uint8_t x_pad = SIDE_WIDTH_LEFT;
    const uint8_t y_pad = TOP_HEIGHT;

    Display::Display() {}

    bool Display::setup() {
        tft.init();
        tft.setRotation(-1);
        tft.fillScreen(TFT_BLACK);
        tft.setTextSize(1);

        renderFull();
    }

    bool Display::renderTitle() {
        tft.fillRect(0, 0, getWidth(), TOP_HEIGHT, TFT_LIGHTGREY);
        tft.setCursor(28, 0, TFT_FONT);
        tft.setTextColor(TFT_BLACK);
        tft.print("Smart Pupillometer");

        return true;
    }

    bool Display::renderLeft() {
        tft.setCursor(1, TOP_HEIGHT+10, SIDEBAR_FONT);
        tft.setTextColor(SCALEBAR_COLOUR);
        tft.print(US_MHZ);
        tft.println(F("MHz"));

        return true;
    }

    bool Display::renderRight() {
        tft.setTextColor(SCALEBAR_COLOUR);

        const uint8_t scalebar_start = TOP_HEIGHT + SCALEBAR_PAD;
        const uint8_t scalebar_end = getHeight() - SCALEBAR_PAD;
        const uint8_t scalebar_x_pos = getWidth() - SIDE_WIDTH_RIGHT + SCALEBAR_PAD;

        // Scale bar
        // TODO: Use drawFastVLine()
        tft.drawLine(scalebar_x_pos, scalebar_start, scalebar_x_pos, scalebar_end, SCALEBAR_COLOUR);

        // Scale bar tick marks with labels
        const uint8_t scalebar_pts = SCALEBAR_TICKS + 2;
        for (uint8_t i = 0; i < scalebar_pts; i++) {
            // Do not display the first and last marker -> out of bounds
            if (i != 0 && i != scalebar_pts) {
                const uint16_t y_pos = getHeight() - i*(getHeight()-TOP_HEIGHT)/scalebar_pts;
                const uint16_t label = i*SCALEBAR_RANGE/scalebar_pts;  // tick label in microseconds

                tft.setCursor(scalebar_x_pos + SCALEBAR_TICK_SIZE+1, y_pos-3, SIDEBAR_FONT);
                tft.print(label);
                // TODO: Use drawFastHLine()
                tft.drawLine(scalebar_x_pos, y_pos,
                             scalebar_x_pos + SCALEBAR_TICK_SIZE-1, y_pos, SCALEBAR_COLOUR);
            }
        }

        // Scale bar title
        tft.setCursor(scalebar_x_pos+3, TOP_HEIGHT, SIDEBAR_FONT);
        tft.print(F("us"));

        return true;
    }

    bool Display::renderColumn(const uint16_t c,
                               const uint16_t scan[IMG_HEIGHT],
                               const uint8_t scale = 1) {
        // Ensure column fits
        if (c >= getWidth()) return false;

        // TODO: Draw entire col at once (e.g. as a bmp/sprite)
        for (uint16_t r = 0; r < IMG_HEIGHT; r++) {
            for (uint8_t rs = 0; rs < scale; rs++) {
                tft.drawPixel(x_pad + c, y_pad + scale*r + rs,
                              scan[IMG_HEIGHT-r-1]); // inverted view
            }
        }

        return true;
    }

    bool Display::renderInner(const uint16_t scan[IMG_WIDTH][IMG_HEIGHT],
                              const uint8_t scale = 1) {
        
        if (scan != NULL) {
            // render full image by sequentially rendering each column
            bool ok = true;
            for (uint16_t c = 0; c < IMG_WIDTH; c++) {
                for (uint8_t cs = 0; cs < scale; cs++) {
                    bool _ok = renderColumn(scale*c + cs, scan[c], scale);
                    if (ok) ok = _ok; // record unsuccessful commands
                }
            }
        } else {
            // clear inner screen
            tft.fillRect(x_pad, y_pad, getWidth() - 2*SIDE_WIDTH,
                         getHeight() - TOP_HEIGHT, TFT_BLACK);
            ok = true;
        }
        
        return ok;
    }

    bool Display::clear() {
        tft.fillScreen(TFT_BLACK);
        return true;
    }

    uint16_t Display::getHeight() { return tft.height(); }
    uint16_t Display::getWidth()  { return tft.width();  }
};
