#pragma once

#include "display/display.hpp"

#define CMD_HANDSHAKE 0
#define CMD_ACK       1
#define CMD_NACK      2
#define CMD_STREAM    3
#define CMD_RESET     4
#define CMD_SETUP     5

#define STATUS_NOT_SETUP 10
#define STATUS_STANDBY   11
#define STATUS_BUSY      12

#define ALLOCATE_TASK_MILLIS 1000

class SerialStream {

   private:

    // Screen properties
    uint8_t  s1_ypos_; // vertical position of "S1" mark on screen sidebar
    uint8_t  s2_ypos_; // vertical position of "S2" mark on screen sidebar

    // USB ports
    uint32_t port_update_ = millis(); // last update time: used for caching
    uint8_t  status_      = STATUS_NOT_SETUP; // device functionality status
    bool     port_prg_    = false; // programming serial port: debug messages and general outputting
    bool     port_usb_    = false; // native serial port: listen for stream and simple outputting

    // Stream properties
    bool raw_signal_ = true; // whether or not to listen for unprocessed signal (raw float values)

    // unions used for transmission of data size across stream of larger than 1 byte
    union u16 {
        byte b[2];
        uint16_t val;
    } u16;

    union i16 {
        byte b[2];
        int16_t val;
    } i16;

    bool toc(uint32_t tic) {
        return millis() - tic > ALLOCATE_TASK_MILLIS;
    }
    
   public:
    Display* display_; // hardware display screen

    SerialStream(Display* display, const uint8_t s1_ypos, const uint8_t s2_ypos) :
        display_(display),
        s1_ypos_(s1_ypos),
        s2_ypos_(s2_ypos) {}
    
    template <size_t N>
    Array<float, N> listen() {
        Array<float, N> arr;
        arr.assign(cfg::numPtsLocal(), 0);

        if (port_usb_ && SerialUSB.available()) {
            uint8_t cmd = SerialUSB.read();

            // deny command if busy
            if (status_ == STATUS_BUSY) {
                SerialUSB.write(CMD_NACK);
                return arr;
            }

            // process command otherwise
            uint32_t tic = millis(); // record task start time for timeout
            switch (cmd) {
                case CMD_HANDSHAKE: {
                    SerialUSB.write(CMD_ACK); // return pong
                    break;
                } case CMD_SETUP: {
                    SerialUSB.write(CMD_ACK); // acknowledge command
                    digitalWrite(LED_BUILTIN, HIGH); // turn on LED during setup processing
                    while (!SerialUSB.available()) if (toc(tic)) return arr; // await stream

                    // populate config array with transmitted 2-byte data, assembled
                    // into unsigned 16-bit integers via LSB (little endianess)
                    uint16_t cfg[cfg::N_CFG] = {}; // based on the amount of expected config params
                    uint8_t i = 0;
                    for (; i < cfg::N_CFG; i++) {
                        SerialUSB.readBytes(u16.b, 2);
                        cfg[i] = u16.val;
                    }
                    cfg::update(cfg); // schedule config update

                    // finish
                    SerialUSB.write(i == cfg::N_CFG ? CMD_ACK : CMD_NACK); // response
                    digitalWrite(LED_BUILTIN, LOW); // turn off LED during standby
                    status_ = STATUS_STANDBY; // enable accepting data streams
                    break;
                } case CMD_STREAM: {
                    // only stream when in standby
                    if (status_ != STATUS_STANDBY) {
                        SerialUSB.write(CMD_NACK);
                        break;
                    }

                    SerialUSB.write(CMD_ACK); // acknowledge command
                    digitalWrite(LED_BUILTIN, HIGH); // turn on LED during stream processing
                    while (!SerialUSB.available()) if (toc(tic)) return arr; // await stream
                    // populate data stream with transmitted 2-byte floats multiplied by 100
                    // and assembled into unsigned 16-bit integers via LSB (little endianess)
                    uint16_t i = 0;
                    for (; i < cfg::numPtsLocal(); i++) {
                        SerialUSB.readBytes(i16.b, 2);
                        arr.push_back((float) i16.val / 100.);
                    }

                    // finish
                    SerialUSB.write(i == cfg::numPtsLocal() ? CMD_ACK : CMD_NACK);
                    digitalWrite(LED_BUILTIN, LOW); // turn off LED during standby
                    break;
                } case CMD_RESET: {
                    display_->current_col = 0; // reset column counter
                    display_->clearInner(); // reset view
                    SerialUSB.write(CMD_ACK);
                    break;
                } default: {
                    // return NACK - unrecognised command
                    SerialUSB.write(CMD_NACK);
                    break;
                }
            }
        }

        return arr;
    }

    void checkConnections(const bool update_display = false) {
        // Handle check only once in a while
        if (millis() - port_update_ < 1000) return;
        port_update_ = millis();

        // Programming USB port (S1)
        if (port_prg_) {
            // connected ...
            if (!Serial) { // TODO: Find a different way of checking connectivity: Handshake?
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
            if (Serial) { // TODO: Find a different way of checking connectivity: Handshake?
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
            if (!SerialUSB) { // TODO: Find a different way of checking connectivity: Handshake?
                // ... disconnected
                port_usb_ = false;
                // Try broadcast to Serial
                if (port_prg_) Serial << F("Native USB connection (S2) disconnected") << endl;
                if (update_display) {
                    display_->setTextColor(display_->colorRed());
                    display_->print(1, s2_ypos_, F("S2")); // update serial status
                    display_->clearInner(); // reset ultrasound image
                    display_->current_col = 0; // reset column counter
                }
            }
        } else {
            // disconnected ...
            if (SerialUSB) { // TODO: Find a different way of checking connectivity: Handshake?
                // ... connected
                port_usb_ = true;
                // Try broadcast to Serial
                if (port_prg_) Serial << F("Found a new native USB connection (S2)") << endl;
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