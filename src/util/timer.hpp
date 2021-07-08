#pragma once
#include <Arduino.h>
#include <Streaming.h>

#define TIMING true  // records and prints timing of heavy operations

class Timer {
   private:
    uint32_t last_millis;  // used for timing heavy operations (for single-threaded use only)

   public:
    /**
     * Initiates timer for heavy tasks if timing feature is enabled.
     */
    inline void start(bool msg = false) {
#if TIMING
        if (msg) Serial << F("Working...") << endl;
#endif
        last_millis = millis();
    }

    /**
     * Stops timer after heavy tasks and prints timing if timing feature is enabled.
     */
    inline uint32_t stop(bool msg = false) {
        uint32_t work = millis() - last_millis;
#if TIMING
        if (msg) Serial << F("Done in ") << work << F(" ms.");
#endif
        return work;
    }
};