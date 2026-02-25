#include <Arduino.h>
#include <WiFi.h>
#include <ESPmDNS.h>
#include "Camera.h"
#include "StreamServer.h"
#include "DisplayModule.h"
#include "NetworkManager.h"
#include "LD2451_Defines.h"
#include "FilterModule.h"
#include "RadarParser.h"
#include "RadarConfig.h"

// --- Radar Default Settings ---
uint8_t cfg_max_dist    = 10; // 0 to 100m
uint8_t cfg_direction   = 2;   // 0: Away, 1: Approach, 2: Both 
uint8_t cfg_min_speed   = 0;   // 0 to 120 km/h 
uint8_t cfg_delay_time  = 0;   // 0 to 255 seconds 
uint8_t cfg_trigger_acc = 1;   // 1 to 10 (Cumulative detections) 
uint8_t cfg_snr_limit   = 0;   // 0 to 64 (Higher = less sensitive) 

// Hardware configuration
const int RADAR_RX_PIN = 21; 
const int RADAR_TX_PIN = 47; 

// Global variables for external access (NetworkManager/Webhook)
uint8_t rawDebugBuffer[64];
int rawDebugLen = 0;
bool debugMode = true;
int globalTargetCount = 0;
bool yoloVetoActive = false;
NetworkManager network;
DisplayModule ui;
Camera myCam;
SignalFilter distFilter;

RadarTarget activeTargets[5];
unsigned long lastValidRadarTime = 0;
const int DATA_PERSIST_MS = 250;

void applyRadarSettings() {
    // Uses RadarConfig handshake (Enable -> Set -> End)
    RadarConfig::sendDefaults(Serial1, cfg_max_dist, cfg_direction, cfg_min_speed, cfg_delay_time, cfg_trigger_acc, cfg_snr_limit);
    delay(100);

    // Final "Start Reporting" command
    uint8_t startCmd[] = {0xFD, 0xFC, 0xFB, 0xFA, 0x04, 0x00, 0x62, 0x00, 0x04, 0x03, 0x02, 0x01};
    Serial1.write(startCmd, sizeof(startCmd));
    Serial1.flush();
}

void setup() {
    // UART1 for Radar
    Serial1.begin(115200, SERIAL_8N1, RADAR_RX_PIN, RADAR_TX_PIN);
    delay(500);

    ui.init();
    ui.updateMessage("BOOTING...", ST77XX_CYAN);

    applyRadarSettings();

    myCam.init();        
    network.init();      
    startCameraServer(); 
    
    if (MDNS.begin("safebaige")) { 
        MDNS.addService("http", "tcp", 80);
        ui.updateMessage("ALLOK", ST77XX_GREEN);
    } else {
        ui.updateMessage("MDNS:ERR", ST77XX_RED);
    }
    
    lastValidRadarTime = millis();
}

void loop() {
    // 1. Let the Parser have first access to Serial1.
    // It will fill rawDebugBuffer if it finds a valid header.
    int newTargets = RadarParser::parse(Serial1, activeTargets, 5, rawDebugBuffer, &rawDebugLen);

    if (newTargets > 0) {
        globalTargetCount = newTargets;
        lastValidRadarTime = millis();
        
        for (int i = 0; i < newTargets; i++) {
            activeTargets[i].smoothedDist = distFilter.smooth(i, (float)activeTargets[i].distance);
        }
        ui.render(newTargets, activeTargets);
    } else {
        // 2. If Parser found nothing, but there's "noise" in Serial, capture it for /debug
        if (debugMode && Serial1.available() > 0) {
            rawDebugLen = 0;
            while(Serial1.available() > 0 && rawDebugLen < 64) {
                rawDebugBuffer[rawDebugLen++] = Serial1.read();
            }
        }

        // Persistence logic: Clear screen if no targets seen for 250ms
        if (millis() - lastValidRadarTime > DATA_PERSIST_MS) {
            if (globalTargetCount > 0) {
                globalTargetCount = 0;
                for (int i = 0; i < 5; i++) distFilter.reset(i);
                ui.render(0, nullptr); 
            }
        }
    }
    
    delay(5);
}