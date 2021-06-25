// Includes
#include <display.h>

// Loggers and debuggers
#define DEBUG true    // keep false unless debugging
#define TIMING true   // records and prints timing of heavy operations

// SPI TFT Display Pins
                        // TFT 1 Vcc -> Arduino Due +3.3V - Power
                        // TFT 2 GND -> Arduino Due GND - Ground
#define PIN_TFT_CS  10  // TFT 3 CS  -> Arduino Due D10 - Chip / slave select
#define PIN_TFT_RST 8   // TFT 4 RST -> Arduino Due D8 - Reset
#define PIN_TFT_DC  9   // TFT 5 DC  -> Arduino Due D9 [A0] - Data / command select
                        // TFT 6 SDA -> Arduino Due SPI:ICSP-4 [MOSI]
                        // TFT 7 SCK -> Arduino Due SPI:ICSP-3 [CSK]
                        // TFT 8 LED -> Arduino Due 3.3V

// On Proteus: A hardware SPI connection was established using:
// --> works with both Adafruit ST7735 library and TFT_eSPI
// ST7735R SDA -> Arduino Mega 51 MOSI    (undefined)   
// ST7735R SCK -> Arduino Mega 52 SCK     (undefined)   
// ST7735R CS  -> Arduino Mega 53 SS      (defined)     [apparently not required]
// ST7735R DC  -> Arduino Mega  8 Digital (defined)     
// ST7735R RST -> Arduino Mega  9 Digital (defined, NC) [Not actually connected**]

// System State Pins
#define PIN_OUT_PULSE    5  // To transmitter (Arduino Due D5)
#define PIN_IN_PULSE     61 // From receiver (Arduino Due D61 [Analog A7])
#define PIN_IN_MAGNITUDE 4  // Switch input for magnification
#define PIN_IN_SLEEP     3  // Switch input for sleep mode

// Variables for internal use
uint32_t lastMillis;  // used for timing heavy operations (for single-threaded use only)

// Internal function declarations
void startTimer();    // starts timing an operation (used for debugging)
uint32_t stopTimer(); // stops timing an operation (used for debugging)

Display display = Display();

/**
 * Initialisation
 */
void setup() {
    Serial.begin(115200);

    display.setup();
}

/**
 * Main tick
 */
void loop() {

}

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