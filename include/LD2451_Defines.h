#ifndef LD2451_DEFINES_H
#define LD2451_DEFINES_H

#include <Arduino.h>

// Protocol Frame Constants 
const uint8_t DATA_FRAME_HEADER[] = {0xF4, 0xF3, 0xF2, 0xF1};
const uint8_t DATA_FRAME_FOOTER[] = {0xF8, 0xF7, 0xF6, 0xF5};

// Struct to hold target data for display and logic
struct RadarTarget {
    uint8_t distance;    // Raw meters from radar
    uint8_t angle;       // Raw 0-255 (128 = 0°)
    bool    approaching; // True if moving toward sensor
    uint8_t speed;       // km/h
    uint8_t snr;         // Signal Quality
    float   smoothedDist;// Float value for FilterModule
};

#endif