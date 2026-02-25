#ifndef RADAR_CONFIG_H
#define RADAR_CONFIG_H

#include <HardwareSerial.h>

class RadarConfig {
public:
    static void sendDefaults(HardwareSerial &ser, uint8_t maxDist, uint8_t direction, uint8_t minSpeed, uint8_t delayTime, uint8_t triggerTimes, uint8_t snrLimit) {
        
        // 1. Enable Configuration (Command 0x00FF)
        // Payload: 0x01 0x00 (Enable)
        uint8_t enableCmd[] = {0xFD, 0xFC, 0xFB, 0xFA, 0x04, 0x00, 0xFF, 0x00, 0x01, 0x00, 0x04, 0x03, 0x02, 0x01};
        ser.write(enableCmd, sizeof(enableCmd));
        ser.flush();
        delay(100); 

        // 2. Set Detection Parameters (Command 0x0002)
        // Length 0x06: 2 bytes Cmd + 4 bytes Payload
        uint8_t paramCmd[] = {
            0xFD, 0xFC, 0xFB, 0xFA, 
            0x06, 0x00,             
            0x02, 0x00,             
            maxDist,                
            direction,              
            minSpeed,               
            delayTime,              
            0x04, 0x03, 0x02, 0x01  
        };
        ser.write(paramCmd, sizeof(paramCmd));
        ser.flush();
        delay(100);

        // 3. Set Sensitivity & Trigger Logic (Command 0x0003)
        // Payload: [Trigger Times] [SNR Threshold] [Reserved 0x00] [Reserved 0x00]
        uint8_t senseCmd[] = {
            0xFD, 0xFC, 0xFB, 0xFA,
            0x06, 0x00,
            0x03, 0x00,             
            triggerTimes,           
            snrLimit,               
            0x00, 0x00,             
            0x04, 0x03, 0x02, 0x01
        };
        ser.write(senseCmd, sizeof(senseCmd));
        ser.flush();
        delay(100);

        // 4. End Configuration (Command 0x00FE)
        // Length 0x02: 2 bytes Cmd + 0 bytes Payload
        uint8_t endCmd[] = {0xFD, 0xFC, 0xFB, 0xFA, 0x02, 0x00, 0xFE, 0x00, 0x04, 0x03, 0x02, 0x01};
        ser.write(endCmd, sizeof(endCmd));
        ser.flush();
        delay(200); // Give it extra time to resume reporting
    }

    static void softReset(HardwareSerial &ser) {
        // Restart Module (Command 0x00A2)
        uint8_t resetCmd[] = {0xFD, 0xFC, 0xFB, 0xFA, 0x02, 0x00, 0xA2, 0x00, 0x04, 0x03, 0x02, 0x01};
        ser.write(resetCmd, sizeof(resetCmd));
        ser.flush();
        delay(500);
    }
};

#endif