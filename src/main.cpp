// Includes
#include "Arduino.h"

// Loggers and debuggers
#define DEBUG true   // keep false unless debugging
#define TIMING true  // records and prints timing of heavy operations

// Variables for internal use
uint32_t lastMillis;  // used for timing heavy operations (for single-threaded use only)

/**
 * Initialisation
 */
void setup() {}

/**
 * Main tick
 */
void loop() {}

/**
 * Initiates timer for heavy tasks if timing feature is enabled.
 */
void startTimer() {
    Serial.println(F("Working..."));
#if TIMING
    lastMillis = millis();
#endif
}

/**
 * Stops timer after heavy tasks and prints timing if timing feature is enabled.
 */
uint32_t stopTimer() {
#if TIMING
    uint32_t work = millis() - lastMillis;

    Serial.print(F("Done in "));
    Serial.print(work);
    Serial.println(F(" ms."));

    return work;
#else
    return 0L;
#endif
}