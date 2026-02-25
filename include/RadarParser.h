#ifndef RADAR_PARSER_H
#define RADAR_PARSER_H

#include <HardwareSerial.h>
#include "LD2451_Defines.h"
#include "FilterModule.h"

extern SignalFilter distFilter;

class RadarParser {
public:
    static int parse(HardwareSerial &ser, RadarTarget *targets, int maxTargets) {
        if (ser.available() < 12) return 0;

        if (ser.peek() == 0xF4) {
            uint8_t header[4];
            ser.readBytes(header, 4);
            
            if (header[1] == 0xF3 && header[2] == 0xF2 && header[3] == 0xF1) {
                uint16_t dataLen;
                ser.readBytes((uint8_t*)&dataLen, 2);

                uint8_t payload[dataLen];
                ser.readBytes(payload, dataLen);

                uint8_t footer[4];
                ser.readBytes(footer, 4);

                int countDetected = payload[0];
                int actualToRead = (countDetected > maxTargets) ? maxTargets : countDetected;

                for (int i = 0; i < actualToRead; i++) {
                    int base = 2 + (i * 5);
                    targets[i].distance    = payload[base + 0];
                    targets[i].angle       = payload[base + 1]; 
                    targets[i].approaching = (payload[base + 2] == 0x00); 
                    targets[i].speed       = payload[base + 3];
                    targets[i].snr         = payload[base + 4];
                    
                    // Apply the filter from filtermodule.h
                    targets[i].smoothedDist = distFilter.smooth(i, (float)targets[i].distance);
                }

                // Reset filters for slots that are no longer active
                for (int i = actualToRead; i < maxTargets; i++) {
                    distFilter.reset(i);
                }

                return actualToRead;
            }
        } else {
            ser.read(); // Skip non-header byte
        }
        return 0;
    }
};

#endif