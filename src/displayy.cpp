#include <displayy.h>

bool Display::setup(const uint16_t image[IMG_WIDTH][IMG_HEIGHT]) {
    init();
    setRotation(1);
    fillScreen(colorBlack());
    setTextSize(1);

    return renderFull(image, IMG_MAG);
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

bool Display::renderColumn(const uint16_t col,
                           const uint16_t scan[IMG_HEIGHT],
                           const uint8_t scale) {
    // Ensure column fits
    if (col >= getWidth()) return false;

    // TODO: Draw entire col at once (e.g. as a bmp/sprite)
    
    // Conventional rendering
    for (uint16_t r = 0; r < IMG_HEIGHT; r++)
        for (uint8_t rs = 0; rs < scale; rs++)
            if (scan[IMG_HEIGHT-r-1] != NULL)
                drawPixel(SIDE_WIDTH_LEFT + col, TOP_HEIGHT + scale*r + rs,
                          scan[IMG_HEIGHT-r-1]); // inverted view

    return true;
}

bool Display::renderInner(const uint16_t scan[IMG_WIDTH][IMG_HEIGHT],
                          const uint8_t scale) {
    
    bool ok = true;

    if (scan == NULL) return ok;

    // render full image by sequentially rendering each column
    for (uint16_t c = 0; c < IMG_WIDTH; c++) {
        for (uint8_t cs = 0; cs < scale; cs++) {
            bool _ok = renderColumn(scale*c + cs, scan[c], scale);
            if (ok) ok = _ok; // record unsuccessful commands
        }
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