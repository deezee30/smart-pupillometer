#pragma once

#include "display/display.hpp"

#define CMD_HANDSHAKE 0
#define CMD_ACK 1
#define CMD_NACK 2
#define CMD_STREAM 3
#define CMD_RESET 4
#define CMD_SETUP 5

#define STATUS_NOT_SETUP 10
#define STATUS_STANDBY 11
#define STATUS_BUSY 12

class SerialStream {

   private:

    // Screen Properties
    Display* display_; // hardware display screen
    Column   last_col_ = col::BLACK; // cached column; used when no new feed occurs
    uint8_t  s1_ypos_; // vertical position of "S1" mark on screen sidebar
    uint8_t  s2_ypos_; // vertical position of "S2" mark on screen sidebar

    // USB ports
    uint32_t port_update_ = millis(); // last update time: used for caching
    uint8_t  status_      = STATUS_NOT_SETUP; // device functionality status
    bool     port_prg_    = false; // programming serial port: debug messages and general outputting
    bool     port_usb_    = false; // native serial port: listen for stream and simple outputting
    
   public:

    SerialStream(Display* display, const uint8_t s1_ypos, const uint8_t s2_ypos) :
        display_(display),
        s1_ypos_(s1_ypos),
        s2_ypos_(s2_ypos) {}
    
    Column listen() {
        Column col(last_col_);

        if (port_usb_ && SerialUSB.available()) {
            uint8_t cmd = SerialUSB.read();

            // deny command if busy
            if (status_ == STATUS_BUSY) {
                SerialUSB.write(CMD_NACK);
                return col;
            }

            // process command otherwise
            switch (cmd) {
                case CMD_HANDSHAKE:
                    SerialUSB.write(CMD_ACK); // return pong
                    break;
                case CMD_SETUP:
                    SerialUSB.write(CMD_ACK);

                    // listen for stream
                    while (!SerialUSB.available());

                    // extract configuration
                    cfg::update(SerialUSB.read(), SerialUSB.read(), SerialUSB.read());

                    // ready
                    SerialUSB.write(CMD_ACK);
                    status_ = STATUS_STANDBY;

                    break;
                case CMD_STREAM:
                    // only stream when in standby
                    if (status_ != STATUS_STANDBY) {
                        SerialUSB.write(CMD_NACK);
                        break;
                    }

                    SerialUSB.write(CMD_ACK);

                    // listen for stream
                    while (!SerialUSB.available());
                    
                    // populate column
                    for (uint16_t r = 0; r < col.max_size(); r++)
                        col[r] = SerialUSB.read();

                    // finished
                    SerialUSB.write(CMD_ACK);
                    // SerialUSB.println((char*) col.data()); // FIXME: Remove

                    break;
                case CMD_RESET:
                    display_->current_col = 0; // reset column counter
                    display_->clearInner(); // reset view
                    SerialUSB.write(CMD_ACK);
                    break;
                default:
                    // return NACK - unrecognised command
                    SerialUSB.write(CMD_NACK);
                    break;
            }
        }

        last_col_ = col;
        return col;
    }

    void checkConnections(const bool update_display = false) {
        // Handle check only once in a while
        if (millis() - port_update_ < 1000) return;
        port_update_ = millis();

        // Programming USB port (S1)
        if (port_prg_) {
            // connected ...
            if (!Serial) {
                // ... disconnected
                port_prg_ = false;
                if (update_display) {
                    display_->setTextColor(display_->colorRed());
                    display_->print(1, s1_ypos_, F("S1")); // update serial status
                    display_->clearInner(); // reset ultrasound image
                    display_->current_col = 0; // reset column counter
                }
            }
        } else {
            // disconnected ...
            if (Serial) {
                // ... connected
                port_prg_ = true;
                Serial << F("Found a new USB connection (S1)") << endl;
                if (update_display) {
                    display_->setTextColor(display_->colorGreen());
                    display_->print(1, s1_ypos_, F("S1")); // update serial status
                    display_->clearInner(); // reset ultrasound image
                    display_->current_col = 0; // reset column counter
                }
            }
        }

        // Native USB port (S2)
        if (port_usb_) {
            // connected ...
            if (!SerialUSB) {
                // ... disconnected
                port_usb_ = false;
                // Try broadcast to Serial
                if (port_prg_) Serial << F("Native USB connection (S2) disconnected") << endl;
                digitalWrite(LED_BUILTIN, LOW); // turn on LED in streaming mode
                if (update_display) {
                    display_->setTextColor(display_->colorRed());
                    display_->print(1, s2_ypos_, F("S2")); // update serial status
                    display_->clearInner(); // reset ultrasound image
                    display_->current_col = 0; // reset column counter
                }
            }
        } else {
            // disconnected ...
            if (SerialUSB) {
                // ... connected
                port_usb_ = true;
                // Try broadcast to Serial
                if (port_prg_) Serial << F("Found a new native USB connection (S2)") << endl;
                digitalWrite(LED_BUILTIN, HIGH); // turn off LED in generating mode
                if (update_display) {
                    display_->setTextColor(display_->colorGreen());
                    display_->print(1, s2_ypos_, F("S2")); // update serial status
                    display_->clearInner(); // reset ultrasound image
                    display_->current_col = 0; // reset column counter
                }
            }
        }

        display_->setTextColor(display_->colorScale()); // reset colour
    }

    bool s1() { return port_prg_; }
    bool s2() { return port_usb_; }
};