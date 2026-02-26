#ifndef RADAR_PARSER_H
#define RADAR_PARSER_H

#include <HardwareSerial.h>
#include "LD2451_Defines.h"
#include "FilterModule.h"

extern SignalFilter distFilter;

class RadarParser {
public:
    static int parse(HardwareSerial &ser, RadarTarget *targets, int maxTargets, uint8_t* debugBuf = nullptr, int* debugLen = nullptr) {
        // Minimum frame is 10 bytes (Header 4 + Len 2 + Footer 4)
        if (ser.available() < 10) return 0;

        if (ser.peek() == 0xF4) {
            uint8_t header[4];
            ser.readBytes(header, 4);
            
            if (header[1] == 0xF3 && header[2] == 0xF2 && header[3] == 0xF1) {
                uint16_t dataLen = 0;
                ser.readBytes((uint8_t*)&dataLen, 2);

                // Update debug buffer for webhook
                if (debugBuf && debugLen) {
                    memcpy(debugBuf, header, 4);
                    memcpy(debugBuf + 4, &dataLen, 2);
                    *debugLen = 6;
                }

                // Handle Empty/Heartbeat frames (F4 F3 F2 F1 00 00 ...)
                if (dataLen == 0) {
                    uint8_t footer[4];
                    ser.readBytes(footer, 4);
                    if (debugBuf && debugLen) {
                        memcpy(debugBuf + 6, footer, 4);
                        *debugLen = 10;
                    }
                    return 0; 
                }

                // Process Payload
                if (dataLen > 100) return 0; // protect stack
                uint8_t payload[dataLen];
                ser.readBytes(payload, dataLen);
                
                if (debugBuf && debugLen) {
                    int toCopy = (dataLen < 50) ? dataLen : 50; 
                    memcpy(debugBuf + *debugLen, payload, toCopy); 
                    *debugLen += toCopy; 
                }

                uint8_t footer[4];
                ser.readBytes(footer, 4);
                if (debugBuf && debugLen && *debugLen < 60) {
                    memcpy(debugBuf + *debugLen, footer, 4);
                    *debugLen += 4;
                }

                int countDetected = payload[0];
                uint8_t alarmInfo = payload[1];

                int actualToRead = (countDetected > maxTargets) ? maxTargets : countDetected;

                for (int i = 0; i < actualToRead; i++) {

                    int base = 2 + (i * 5); 

                    if (base + 4 < dataLen) {

                        uint8_t rawAngle = payload[base + 0];
                        targets[i].angle = (int)rawAngle - 0x80;  // convert to signed degrees

                        targets[i].distance = payload[base + 1];

                        uint8_t dirByte = payload[base + 2];
                        targets[i].approaching = (dirByte == 0x00);  // 00 = approaching

                        targets[i].speed = payload[base + 3];
                        targets[i].snr   = payload[base + 4];

                        targets[i].smoothedDist =
                            distFilter.smooth(i, (float)targets[i].distance);
                    }
                }

                // Reset unused slots
                for (int i = actualToRead; i < maxTargets; i++) {
                    distFilter.reset(i);
                }

                return actualToRead;
            }
        } else {
            ser.read(); // Discard non-header byte to find next alignment
        }
        return 0;
    }
};

#endif