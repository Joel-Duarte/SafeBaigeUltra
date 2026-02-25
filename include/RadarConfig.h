#ifndef RADAR_CONFIG_H
#define RADAR_CONFIG_H

#include <HardwareSerial.h>

class RadarConfig {
public:
    static void sendDefaults(HardwareSerial &ser, uint8_t maxDist, uint8_t direction, uint8_t minSpeed, uint8_t delayTime, uint8_t triggerTimes, uint8_t snrLimit) {
        // Enable Configuration (Command 0x00FF)
        // Must be sent before any other configuration
        uint8_t enableCmd[] = {0xFD, 0xFC, 0xFB, 0xFA, 0x04, 0x00, 0xFF, 0x00, 0x01, 0x00, 0x04, 0x03, 0x02, 0x01};
        ser.write(enableCmd, sizeof(enableCmd));
        delay(100); 

        // Set Detection Parameters (Command 0x0002)
        // Payload: [Max Dist (0x0A-0xFF)] [Direction (0-2)] [Min Speed (0-0x78)] [Delay (0-0xFF)] 
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
        delay(100);

        // Set Sensitivity & Cumulative Targeting (Command 0x0003)
        // Payload: [Trigger Times (1-10)] [SNR Threshold (0-64)] [0x00] [0x00] 
        uint8_t senseCmd[] = {
            0xFD, 0xFC, 0xFB, 0xFA,
            0x06, 0x00,
            0x03, 0x00,             
            triggerTimes,           // Cumulative effective trigger times (1-0x0A) 
            snrLimit,               // SNR threshold 
            0x00, 0x00,             
            0x04, 0x03, 0x02, 0x01
        };
        ser.write(senseCmd, sizeof(senseCmd));
        delay(100);

        // End Configuration (Command 0x00FE)
        // Resumes normal radar working mode
        uint8_t endCmd[] = {0xFD, 0xFC, 0xFB, 0xFA, 0x02, 0x00, 0xFE, 0x00, 0x04, 0x03, 0x02, 0x01};
        ser.write(endCmd, sizeof(endCmd));
        delay(100);
    }
};

#endif