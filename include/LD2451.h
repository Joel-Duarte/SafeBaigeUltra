#ifndef LD2451_H
#define LD2451_H

#include <HardwareSerial.h>
#include <vector>
#include "LD2451_Defines.h"

// Global variables defined in main.cpp for the webhook
extern uint8_t rawDebugBuffer[64];
extern int rawDebugLen;
extern bool debugMode;

class LD2451 {
public:
    // Uses the clean pins confirmed safe from camera interference
    LD2451(HardwareSerial& serial) : 
        _radarSerial(serial), 
        _rxPin(21), 
        _txPin(47), 
        _baud(115200) {}

    void begin();
    int update(RadarTarget* targets, int maxTargets);

    // Configuration wrappers
    bool configure(uint8_t maxDist, uint8_t dir, uint8_t minSpd, uint8_t trigger, uint8_t snr);
    bool restart();

private:
    HardwareSerial& _radarSerial;
    int _rxPin;
    int _txPin;
    long _baud;

    void _sendCmd(LD2451_Cmd cmd, const uint8_t* data, uint16_t len);
};

#endif