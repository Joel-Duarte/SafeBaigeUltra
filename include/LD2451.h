#ifndef LD2451_H
#define LD2451_H

#include <HardwareSerial.h>
#include <vector>
#include "LD2451_Defines.h"

// --- Variables from main.cpp ---
extern const int RADAR_TX_PIN;
extern const int RADAR_RX_PIN;
extern uint8_t rawDebugBuffer[64];
extern int rawDebugLen;
extern bool debugMode;

class LD2451 {
public:
    // Constructor now uses the global variables directly
    LD2451(HardwareSerial& serial) : 
        _radarSerial(serial), 
        _baud(115200) {}

    void begin() {
        // Uses the pins defined in main.cpp
        _radarSerial.begin(_baud, SERIAL_8N1, RADAR_RX_PIN, RADAR_TX_PIN);
    }

    int update(RadarTarget* targets, int maxTargets);

    // Configuration wrappers
    bool configure(uint8_t maxDist, uint8_t dir, uint8_t minSpd, uint8_t trigger, uint8_t snr);
    bool restart();

private:
    HardwareSerial& _radarSerial;
    long _baud;

    void _sendCmd(LD2451_Cmd cmd, const uint8_t* data, uint16_t len);
};

#endif