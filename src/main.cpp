// Arduino Due
// Microcontroller: (Atmel) AT91SAM3X8E 32-bit ARM Cortex-M3
// Clock speed: 84 MHz
// Flash Memory: 512 KB
// SRAM: 96 KB (two banks; 64 KB + 32 KB)

// Includes
#include "signal_processor.hpp"
#include "display/display_st7735.hpp"
#include "util/timer.hpp"
#include "serial_server.hpp"

// Loggers and debuggers
#define DEBUG false    // keep false unless debugging

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
DisplayST7735 display(cfg::imgScale()); // create TFT ST7735 display instance
uint16_t total_cols = display.getColumns();

// Serial communication manager
SerialStream usb(&display, S1_YPOS, S2_YPOS);

// Scanning timing
Timer scan_timer;
uint16_t scan_time = 0;
uint16_t scan_time_n = 0;

/**
 * Initialisation
 */
void setup() {
    // Configure pins
    pinMode(LED_BUILTIN, OUTPUT);
    pinMode(PIN_OUT_PULSE, OUTPUT);
    pinMode(PIN_IN_PULSE, INPUT);
    pinMode(PIN_IN_MAGNITUDE, INPUT);
    pinMode(PIN_IN_SLEEP, INPUT);
    
    // Set up Serials
    Serial.begin(115200);
    SerialUSB.begin(115200);
    usb.checkConnections(false); // do premature checking in case startup messages to be printed

    // Set up display screen
    display.setup();

    // Show "non-connected" initial serial states
    display.setTextColor(display.colorRed());
    display.print(1, S1_YPOS, F("S1"));
    display.print(1, S2_YPOS, F("S2"));
    display.setTextColor(display.colorScale());
}

/**
 * Main tick
 */
void loop() {
    // Check if config is scheduled to update through port
    if (cfg::scheduledUpdate()) {
        // Update frequency setting
        display.renderLeft();
        // TODO: Update acquisition time

        cfg::finishUpdate();
    }
    
    // Actively listen for changes in serial connectivity
    usb.checkConnections(true);

    // Disable signal retrieval while paused
    //while (digitalRead(PIN_IN_SLEEP) == LOW) ;

    scan_timer.start(); // start timer
    // Generate AScan from ultrasound data stream or otherwise
    Column scan = SignalProcessor::receiveAScan(display.getRows(), &usb);
    display.renderColumn(scan); // render on hardware screen at current column
    scan_time += scan_timer.stop(); // stop timer and save
    scan_time_n++;

    // Average out timer and output progress
    if (display.current_col == 0) {
        scan_time = 0;
        scan_time_n = 0;
#if DEBUG
        Serial << F("Rendering ") << total_cols << F(" scans.") << endl;
#endif
    } else if ((display.current_col+1) % round(total_cols/5) == 0 ||  // progress
                display.current_col+1 == total_cols) {                // end
#if DEBUG
        Serial << F("Rendered ") << current_col+1 << F("/") << total_cols << F(" scans (")
               << 100*(current_col+1)/total_cols << F("%) in ") << scan_time
               << F(" ms (~") << scan_time/scan_time_n << F(" ms/scan).") << endl;
#endif
        scan_time = 0;
        scan_time_n = 0;
    }

    if (++display.current_col == total_cols) {
        display.current_col = 0;
        display.clearInner(); // clear when new data comes in
    }
}