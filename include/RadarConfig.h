#ifndef RADAR_CONFIG_H
#define RADAR_CONFIG_H

#include <HardwareSerial.h>

class RadarConfig {
public:
    static void sendDefaults(HardwareSerial &ser, uint8_t maxDist, uint8_t direction, uint8_t minSpeed, uint8_t delayTime, uint8_t triggerTimes, uint8_t snrLimit) {
        
        // 1. Enable Configuration
        uint8_t enableCmd[] = {0xFD, 0xFC, 0xFB, 0xFA, 0x04, 0x00, 0xFF, 0x00, 0x01, 0x00, 0x04, 0x03, 0x02, 0x01};
        ser.write(enableCmd, sizeof(enableCmd));
        ser.flush();
        delay(100); 

        // 2. Set Detection Parameters (Command 0x0002)
        // Ranges[cite: 321, 322, 323]: Dist(0-100), Speed(0-120), Delay(1-30)
        uint8_t paramCmd[] = {
            0xFD, 0xFC, 0xFB, 0xFA, 0x06, 0x00, 0x02, 0x00,             
            maxDist, direction, minSpeed, delayTime,              
            0x04, 0x03, 0x02, 0x01  
        };
        ser.write(paramCmd, sizeof(paramCmd));
        ser.flush();
        delay(100);

        // 3. Set Sensitivity & Trigger Logic (Command 0x0003)
        // Ranges[cite: 326]: Trigger(1-10), SNR(1-255)
        uint8_t senseCmd[] = {
            0xFD, 0xFC, 0xFB, 0xFA, 0x06, 0x00, 0x03, 0x00,             
            triggerTimes, snrLimit, 0x00, 0x00,             
            0x04, 0x03, 0x02, 0x01
        };
        ser.write(senseCmd, sizeof(senseCmd));
        ser.flush();
        delay(100);

        // 4. End Configuration
        uint8_t endCmd[] = {0xFD, 0xFC, 0xFB, 0xFA, 0x02, 0x00, 0xFE, 0x00, 0x04, 0x03, 0x02, 0x01};
        ser.write(endCmd, sizeof(endCmd));
        ser.flush();
        delay(200); 
    }

    static void softRestart(HardwareSerial &ser) {
        // Restart Module (Command 0x00A3)
        uint8_t restartCmd[] = {0xFD, 0xFC, 0xFB, 0xFA, 0x02, 0x00, 0xA3, 0x00, 0x04, 0x03, 0x02, 0x01};
        ser.write(restartCmd, sizeof(restartCmd));
        ser.flush();
        delay(500);
    }

    static void factoryReset(HardwareSerial &ser) {
        // Restore Factory Settings (Command 0x00A2)
        uint8_t factoryCmd[] = {0xFD, 0xFC, 0xFB, 0xFA, 0x02, 0x00, 0xA2, 0x00, 0x04, 0x03, 0x02, 0x01};
        ser.write(factoryCmd, sizeof(factoryCmd));
        ser.flush();
        delay(100);
        softRestart(ser); // Changes take effect after restart
    }
};

#endif