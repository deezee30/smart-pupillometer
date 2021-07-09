#include "display.hpp"

bool Display::setup() {
    init();
    setRotation(1);
    fillScreen(colorBlack());
    setTextSize(1);

    return renderDefault();
}

bool Display::setup(const Image scan) {
    return setup() & renderInner(scan, IMG_MAG);
}

bool Display::renderTitle() {
    fillRect(0, 0, getWidth(), TOP_HEIGHT, colorLightGrey());
    setTextFont(fontTitle());
    setTextColor(colorBlack());
    print(28, 0, F("Smart Pupillometer"));

    return true;
}

bool Display::renderLeft() {
    setTextFont(fontContent());
    setTextColor(colorScale());
    int16_t pad = print(1, TOP_HEIGHT+10, us_freq);
    print(2 + pad, TOP_HEIGHT+10, F("MHz"));

    return true;
}

bool Display::renderRight() {
    setTextFont(fontContent());
    setTextColor(colorScale());

    const uint8_t scalebar_start = TOP_HEIGHT  + SCALEBAR_PAD;
    const uint8_t scalebar_end   = getHeight() - SCALEBAR_PAD;
    const uint8_t scalebar_x_pos = getWidth()  - SIDE_WIDTH_RIGHT + SCALEBAR_PAD;

    // Scale bar
    drawFastVLine(scalebar_x_pos, scalebar_start,
                  scalebar_end - scalebar_start, colorScale());

    // Scale bar tick marks with labels
    const uint8_t scalebar_pts = SCALEBAR_TICKS + 2;
    for (uint8_t i = 0; i < scalebar_pts; i++) {
        // Do not display the first and last marker -> out of bounds
        if (i != 0 && i != scalebar_pts) {
            const uint16_t y_pos = getHeight() - i*(getHeight()-TOP_HEIGHT)/scalebar_pts;
            const uint16_t label = i*SCALEBAR_RANGE/scalebar_pts;  // tick label in microseconds

            print(scalebar_x_pos + SCALEBAR_TICK_SIZE+2, y_pos-3, label);
            drawFastHLine(scalebar_x_pos+1, y_pos, SCALEBAR_TICK_SIZE, colorScale());
        }
    }

    // Scale bar title
    print(scalebar_x_pos+3, TOP_HEIGHT, F("us"));

    return true;
}

bool Display::renderColumn(const uint16_t c,
                           const Column scan,
                           const uint8_t scale) {
    // Ensure column fits
    if (c >= getWidth()) return false;

    // TODO: Draw entire col at once (e.g. as a bmp/sprite)
    
    // Conventional rendering
    for (uint16_t cs = 0; cs < scale; cs++)
        for (uint16_t r = 0; r < IMG_HEIGHT; r++)
            for (uint8_t rs = 0; rs < scale; rs++)
                drawPixel(SIDE_WIDTH_LEFT + scale*c + cs, // scaled, padded column
                          TOP_HEIGHT      + scale*r + rs, // scaled, padded row
                          grayRGB565To16(scan[IMG_HEIGHT-r-1])); // inverted view

    return true;
}

bool Display::renderInner(const Image scan,
                          const uint8_t scale) {
    
    bool ok = true;

    // render full image by sequentially rendering each column
    for (uint16_t c = 0; c < IMG_WIDTH; c++) {
        bool _ok = renderColumn(c, scan[c], scale);
        if (ok) ok = _ok; // record unsuccessful commands
    }
    
    return ok;
}

bool Display::clear() {
    fillScreen(colorBlack());
    return true;
}

bool Display::clearInner() {
    fillRect(SIDE_WIDTH_LEFT, TOP_HEIGHT, IMG_WIDTH, IMG_HEIGHT, colorBlack());
    return true;
}