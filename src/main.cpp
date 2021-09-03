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

// Options (Left Sidebar)
#define US_MHZ           10 // Ultrasound probe operational frequency [MHz]
#define IMG_MAG          1  // Linear amplification of image

// Positioning (Left Sidebar)
#define S1_YPOS          100 // Programming USB port (S1) y positioning
#define S2_YPOS          (S1_YPOS+10) // Native USB port (S2) y positioning

// Variables for internal use
DisplayST7735 display(US_MHZ, IMG_MAG); // create TFT ST7735 display instance
uint16_t current_col = 0; // current scanning column
uint16_t total_cols = display.getColumns();
uint32_t port_update = millis();

// Scanning timing
Timer scan_timer;
uint16_t scan_time = 0;
uint16_t scan_time_n = 0;

static const float e[] = {0, 1.3, 3.2, 3.8, 5, 6, 7}; // microseconds
static const Array<float, 7> echos(e);

bool port_prg = false; // Programming serial port: Debug messages and general outputting
bool port_usb = false; // Native serial port: Listen for data stream and simple outputting

// Function declarations
void check_serials(bool update_display = false);

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
    check_serials(false); // do premature checking in case startup messages need to be printed

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
    
    // Actively listen for changes in serial connectivity
    check_serials(true);

    // TODO: Disable scanning while paused
    //while (digitalRead(PIN_IN_SLEEP) == HIGH) ;

    scan_timer.start(); // start timer
    Column scan;
    if (port_usb) {
        // Native USB is enabled - set up stream

    } else {
        // Generate A scan
        scan = Demodulator::generateAScan(echos, current_col, display.getRows());
    }

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
               << F(" ms (~") << scan_time/scan_time_n << F(" ms/scan).") << endl;
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

void check_serials(bool update_display) {
    // Handle check only once in a while
    if (millis() - port_update < 1000) return;
    port_update = millis();

    // Programming USB port (S1)
    if (port_prg) {
        // connected ...
        if (!Serial) {
            // ... disconnected
            port_prg = false;
            // Try broadcast to SerialUSB
            if (port_usb) SerialUSB << F("USB connection (S1) disconnected") << endl;
            if (update_display) {
                // update sidebar
                display.setTextColor(display.colorRed());
                display.print(1, S1_YPOS, F("S1"));
            }
        }
    } else {
        // disconnected ...
        if (Serial) {
            // ... connected
            port_prg = true;
            Serial << F("Found a new USB connection (S1)") << endl;
            if (update_display) {
                // update sidebar
                display.setTextColor(display.colorGreen());
                display.print(1, S1_YPOS, F("S1"));
            }
        }
    }

    // Native USB port (S2)
    if (port_usb) {
        // connected ...
        if (!SerialUSB) {
            // ... disconnected
            port_usb = false;
            // Try broadcast to Serial
            if (port_prg) Serial << F("Native USB connection (S2) disconnected") << endl;
            digitalWrite(LED_BUILTIN, LOW); // turn on LED in streaming mode
            if (update_display) {
                // update sidebar
                display.setTextColor(display.colorRed());
                display.print(1, S2_YPOS, F("S2"));
            }
        }
    } else {
        // disconnected ...
        if (SerialUSB) {
            // ... connected
            port_usb = true;
            // Try broadcast to Serial. If offline then broadcast to SerialUSB
            if (port_prg) Serial << F("Found a new native USB connection (S2)") << endl;
            else SerialUSB << F("Found a new native USB connection (S2)") << endl;
            digitalWrite(LED_BUILTIN, HIGH); // turn off LED in generating mode
            if (update_display) {
                // update sidebar
                display.setTextColor(display.colorGreen());
                display.print(1, S2_YPOS, F("S2"));
            }
        }
    }

    display.setTextColor(display.colorScale()); // reset colour
}