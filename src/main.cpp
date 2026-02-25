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
uint8_t cfg_max_dist    = 5; // 0 to 100m
uint8_t cfg_direction   = 1;   // 0: Away, 1: Approach, 2: Both 
uint8_t cfg_min_speed   = 0;   // 0 to 120 km/h 
uint8_t cfg_delay_time  = 0;   // 0 to 255 seconds 
uint8_t cfg_trigger_acc = 1;   // 1 to 10 (Cumulative detections) 
uint8_t cfg_snr_limit   = 0;   // 0 to 64 (Higher = less sensitive) 

void applyRadarSettings() {
    RadarConfig::sendDefaults(Serial1, cfg_max_dist, cfg_direction, cfg_min_speed, cfg_delay_time, cfg_trigger_acc, cfg_snr_limit);
}

const int RADAR_RX_PIN = 1; 
const int RADAR_TX_PIN = 2; 

NetworkManager network;
DisplayModule ui;
Camera myCam;
SignalFilter distFilter;

RadarTarget activeTargets[5];
int globalTargetCount = 0;
bool yoloVetoActive = false;
unsigned long lastValidRadarTime = 0;
const int DATA_PERSIST_MS = 250;

void setup() {
    // Start Radar Hardware
    Serial1.begin(115200, SERIAL_8N1, RADAR_RX_PIN, RADAR_TX_PIN);
    
    ui.init();
    ui.updateMessage("BOOTING...", ST77XX_CYAN);

    // Apply hardware settings
    applyRadarSettings();

    myCam.init();        
    network.init();      
    startCameraServer(); 
    
    if (!MDNS.begin("safebaige")) { 
        ui.updateMessage("MDNS:ERR", ST77XX_RED);
    } else {
        delay(100);
        if(MDNS.addService("http", "tcp", 80)) {
            ui.updateMessage("MDNS:OK", ST77XX_GREEN);
        } else {
            ui.updateMessage("MDNS:ERR", ST77XX_ORANGE);
        }
    }
    
    ui.updateMessage("ALLOK", ST77XX_GREEN);
    lastValidRadarTime = millis();
}

void loop() {
    int newTargets = RadarParser::parse(Serial1, activeTargets, 5);

    if (newTargets > 0) {
        globalTargetCount = newTargets;
        lastValidRadarTime = millis();
        ui.render(globalTargetCount, activeTargets);
    } else {
        if (millis() - lastValidRadarTime > DATA_PERSIST_MS) {
            if (globalTargetCount > 0) {
                globalTargetCount = 0;
                
                for (int i = 0; i < 5; i++) {
                    distFilter.reset(i);
                }
                
                ui.render(0, nullptr); 
            }
        }
    }
    
    delay(10);
}