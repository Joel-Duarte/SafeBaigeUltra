#ifndef LD2451_H
#define LD2451_H

#include <HardwareSerial.h>
#include <vector>
#include "LD2451_Defines.h"

// --- Hardware Configuration Variables ---
static int RADAR_RX_PIN = 1;      // ESP RX (Connected to Radar TX)
static int RADAR_TX_PIN = 2;      // ESP TX (Connected to Radar RX)
static long RADAR_BAUD  = 115200; // Default factory baud rate

class LD2451 {
public:
    // Pass the serial peripheral you want to use (e.g., Serial1 or Serial2)
    LD2451(HardwareSerial& serial);

    // Initialization using the variables defined above
    void begin();
    
    // Main processing loop - call this in your main loop()
    void update();

    // --- Configuration Methods (For WebSocket POST commands) ---
    // These methods handle the "Enable Config -> Command -> End Config" sequence
    bool setDetectionRange(uint8_t maxMeters);
    bool setSensitivity(uint8_t snrThreshold);
    bool setDirectionFilter(uint8_t mode); // 0: Away, 1: Approach, 2: Both
    bool applySystemRestart();

    // --- Data Accessors (For Screen & Passing Logic) ---
    bool isVehicleDetected() const { return _targetDetected; }
    const std::vector<RadarTarget>& getTargets() const { return _targets; }
    
    // Logic helper: returns true if any car is currently approaching at high speed
    bool hasApproachingThreat(uint8_t speedThreshold) const;

private:
    HardwareSerial& _radarSerial;
    std::vector<RadarTarget> _targets;
    bool _targetDetected = false;
    uint8_t _rawBuffer[64];
    
    // Protocol handling
    void _parseIncomingByte(uint8_t byte);
    void _processDataFrame(uint8_t* data, size_t len);
    void _sendProtocolCmd(LD2451_Cmd cmd, const uint8_t* data, uint16_t len);
    bool _enterConfigMode(bool enable);
};

#endif