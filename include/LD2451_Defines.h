#ifndef LD2451_DEFINES_H
#define LD2451_DEFINES_H

#include <Arduino.h>

// Protocol Frame Constants 
const uint8_t CMD_FRAME_HEADER[] = {0xFD, 0xFC, 0xFB, 0xFA};
const uint8_t CMD_FRAME_FOOTER[] = {0x04, 0x03, 0x02, 0x01};
const uint8_t DATA_FRAME_HEADER[] = {0xF4, 0xF3, 0xF2, 0xF1};
const uint8_t DATA_FRAME_FOOTER[] = {0xF8, 0xF7, 0xF6, 0xF5};

// Command Words 
enum LD2451_Cmd : uint16_t {
    CMD_ENABLE_CONFIG  = 0x00FF, // Value: 0x0001 to enable 
    CMD_END_CONFIG     = 0x00FE, // Resume work mode 
    CMD_SET_PARAMS     = 0x0002, // Max dist, direction, min speed, delay 
    CMD_READ_PARAMS    = 0x0012, // [cite: 23]
    CMD_SET_SENSITIVE  = 0x0003, // Trigger count, SNR threshold
    CMD_BAUD_RATE      = 0x00A1, // 1-8 index 
    CMD_RESTART        = 0x00A2  // 
};

// Configuration Values
enum LD2451_Direction : uint8_t {
    DIR_AWAY      = 0x00,
    DIR_APPROACH  = 0x01,
    DIR_BOTH      = 0x02
};

struct RadarTarget {
    int8_t angle;       // Actual angle = Report - 0x80 
    uint8_t distance;   // Unit: m 
    uint8_t direction;  // 00: Close, 01: Far 
    uint8_t speed;      // Unit: km/h 
    uint8_t snr;        // 0~255 
};

#endif