// Arduino Due
// Microcontroller: (Atmel) AT91SAM3X8E 32-bit ARM Cortex-M3
// Clock speed: 84 MHz
// Flash Memory: 512 KB
// SRAM: 96 KB (two banks; 64 KB + 32 KB)

// Includes
#include <display_st7735.h>
#include <image.h>
#include <demodulator.h>

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

// Options (Left Sidebar)
#define US_MHZ           5  // Ultrasound probe operational frequency [MHz]

// Internal function declarations
void startTimer();    // starts timing an operation (used for debugging)
uint32_t stopTimer(); // stops timing an operation (used for debugging)

// Variables for internal use
DisplayST7735 display = DisplayST7735(US_MHZ); // create TFT ST7735 display instance
uint32_t last_millis;  // used for timing heavy operations (for single-threaded use only)
uint16_t current_col = 0; // current scanning column
//uint16_t **image_565;

/**
 * Initialisation
 */
void setup() {
    // Init Serial and wait to connect
    Serial.begin(115200);
    while (!Serial);
    Serial.println();

    display.setup(); // set up display and show initial image

    //static const float echos[] = {0, 1.3, 3.2, 3.8, 5, 6, 7};  // microseconds
    //image_565 = Demodulator::generateBScan(echos, 7);

    delay(50); // post-setup settling time
}

/**
 * Main tick
 */
void loop() {

    // Perform column scan
    display.renderColumn(current_col, image[current_col++]);

    if (current_col == IMG_WIDTH) {
        // TODO: Remove
        while (true);

        current_col = 0;
        // TODO: Remove when new data comes in
        display.clearInner();
    }
}

/**
 * Initiates timer for heavy tasks if timing feature is enabled.
 */
void startTimer() {
    Serial.println(F("Working..."));
#if TIMING
    last_millis = millis();
#endif
}

/**
 * Stops timer after heavy tasks and prints timing if timing feature is enabled.
 */
uint32_t stopTimer() {
#if TIMING
    uint32_t work = millis() - last_millis;

    Serial.print(F("Done in "));
    Serial.print(work);
    Serial.println(F(" ms."));

    return work;
#else
    return 0L;
#endif
}
