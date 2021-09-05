#pragma once

#include "Arduino.h"

namespace cfg {

    // Default options
    namespace def {
        static const uint8_t  FREQUENCY      = 10; // ultrasound probe operational frequency [MHz]
        static const uint8_t  IMG_SCALE      = 1; // linear amplification of image
        static const uint16_t SAMP_RATE      = 100;
        static const uint16_t NUM_PTS_GLOBAL = 1000;
        static const uint16_t MIN_T          = 0;
        static const uint16_t MAX_T          = 1000;
        static const uint8_t  GAIN           = 10;
        static const uint16_t SPEED_SOUND    = 1550;
        static const bool     BSCAN_DP       = true;
        static const uint8_t  SELECT_PLANE   = 40;
    }

    static const uint8_t N_CFG = 10; // number of config params to load
    inline bool config_update_ = false; // scheduled update of configuration settings
    inline uint16_t config_[N_CFG] = {
        def::FREQUENCY, def::IMG_SCALE, def::SAMP_RATE, def::NUM_PTS_GLOBAL,
        def::MIN_T,     def::MAX_T,     def::GAIN,      def::SPEED_SOUND,
        def::BSCAN_DP,  def::SELECT_PLANE,
    };

    inline void update(uint16_t config[N_CFG]) {
        for (uint8_t i = 0; i < N_CFG; i++)
            config_[i] = config[i];
        config_update_ = true;
    }

    inline void finishUpdate()      { config_update_ = false; }
    inline bool scheduledUpdate()   { return config_update_; }
    inline uint8_t  freq()          { return config_[0]; }
    inline uint8_t  imgScale()      { return config_[1]; }
    inline uint16_t sampRate()      { return config_[2]; }
    inline uint16_t numPtsGlobal()  { return config_[3]; }
    inline uint16_t minT()          { return config_[4]; }
    inline uint16_t maxT()          { return config_[5]; }
    inline uint8_t  gain()          { return config_[6]; }
    inline uint16_t numPtsLocal()   { return maxT()-minT(); }
    inline float    acqTime()       { return (float) numPtsGlobal() / (float) sampRate(); }
    inline uint16_t speedSound()    { return config_[7]; }
    inline bool     bscanDP()       { return config_[8]; }
    inline uint8_t  selectPlane()   { return config_[9]; }
}