// Arduino Due
// Microcontroller: (Atmel) AT91SAM3X8E 32-bit ARM Cortex-M3
// Clock speed: 84 MHz
// Flash Memory: 512 KB
// SRAM: 96 KB (two banks; 64 KB + 32 KB)

// Includes
#include "display/screen.hpp"
#include "display/display_st7735.hpp"
#include <Streaming.h>
#include "util/timer.hpp"

// Loggers and debuggers
#define DEBUG true    // keep false unless debugging

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
#define US_MHZ           10 // Ultrasound probe operational frequency [MHz]
#define IMG_MAG          8  // Linear amplification of image

// Variables for internal use
DisplayST7735 display(US_MHZ, IMG_MAG); // create TFT ST7735 display instance
uint16_t current_col = 0; // current scanning column
uint16_t total_cols = display.getColumns();

// Scanning timing
Timer scan_timer;
uint16_t scan_time = 0;
uint16_t scan_time_n = 0;

static const float e[] = {0, 1.3, 3.2, 3.8, 5, 6, 7}; // microseconds
static const Array<float, 7> echos(e);

/**
 * Initialisation
 */
void setup() {
    // Init Serial and wait to connect
    Serial.begin(115200);
    while (!Serial);
    Serial << endl;

    display.setup();

#if DEBUG
    Serial << F("Setup finished.") << endl;
#endif

    delay(50); // post-setup settling time
}

/**
 * Main tick
 */
void loop() {

    // TODO: Disable scanning while paused
    while (digitalRead(PIN_IN_SLEEP) == HIGH) ;

    // Perform column scan
    scan_timer.start(); // start timer
    Column scan = Demodulator::generateAScan(echos, current_col, display.getRows());
    display.renderColumn(current_col, scan);
    scan_time += scan_timer.stop(); // stop timer and save
    scan_time_n++;

    // Average out timer and output progress
    if (current_col == 0) {
        scan_time = 0;
        scan_time_n = 0;
#if DEBUG
        Serial << F("Rendering ") << total_cols << F(" scans.") << endl;
#endif
    } else if ((current_col+1) % round(total_cols/5) == 0 ||  // progress
                current_col+1 == total_cols) {                // end
#if DEBUG
        Serial << F("Rendered ") << current_col+1 << F("/") << total_cols << F(" scans (")
               << 100*(current_col+1)/total_cols << F("%) in ") << scan_time
               << F(" ms (~") << scan_time/scan_time_n << F(" ms/col).") << endl;
#endif
        scan_time = 0;
        scan_time_n = 0;
    }

    if (++current_col == total_cols) {
        current_col = 0;
        // TODO: Remove when new data comes in
        //display.clearInner();
    }
}