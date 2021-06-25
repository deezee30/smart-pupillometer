#pragma once

// Includes
#include <Arduino.h>

class Display {

   public:

    Display();

    bool setup();

    bool renderTitle();

    bool renderLeft();

    bool renderRight();
    
    bool renderInner(const float **scan,
                     const uint8_t scale = 1);

    bool renderColumn(const uint16_t c,
                      const uint16_t *scan,
                      const uint8_t scale = 1);
    
    bool renderFull(const float **inner = NULL,
                    const uint8_t scale = 1) {
        return renderTitle()              // Draw heading: Title
             & renderLeft()               // Draw left sidebar: Options
             & renderRight()              // Draw right sidebar: Scale
             & renderInner(inner, scale); // Refresh inner view with what's available
    }

    bool clear();

    static uint16_t getHeight();

    static uint16_t getWidth();

};