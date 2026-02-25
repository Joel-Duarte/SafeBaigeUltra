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
uint8_t cfg_max_dist    = 10; 
uint8_t cfg_direction   = 2;   
uint8_t cfg_min_speed   = 0;   
uint8_t cfg_delay_time  = 0;   
uint8_t cfg_trigger_acc = 1;   
uint8_t cfg_snr_limit   = 0;   

// --- Hardware & System Configuration ---
const int RADAR_TX_PIN = 1; 
const int RADAR_RX_PIN = 2; 
const int DATA_PERSIST_MS = 800; 
uint16_t cfg_cam_trigger_ms = 2000; 

// --- Global Variables ---
uint8_t rawDebugBuffer[64];
int rawDebugLen = 0;
bool debugMode = true;
int globalTargetCount = 0;
bool yoloVetoActive = false;
unsigned long trackingStartTime = 0;

// --- Module Instances ---
NetworkManager network;
DisplayModule ui;
Camera myCam;
SignalFilter distFilter;
RadarTarget activeTargets[5];
unsigned long lastValidRadarTime = 0;

void applyRadarSettings() {
    RadarConfig::sendDefaults(Serial1, cfg_max_dist, cfg_direction, cfg_min_speed, cfg_delay_time, cfg_trigger_acc, cfg_snr_limit);
    delay(100);
    uint8_t startCmd[] = {0xFD, 0xFC, 0xFB, 0xFA, 0x04, 0x00, 0x62, 0x00, 0x04, 0x03, 0x02, 0x01};
    Serial1.write(startCmd, sizeof(startCmd));
    Serial1.flush();
}

void setup() {
    // Correct Pins: RX=2, TX=1
    Serial1.begin(115200, SERIAL_8N1, RADAR_RX_PIN, RADAR_TX_PIN);
    delay(500);

    ui.init();
    ui.updateMessage("BOOTING", ST77XX_CYAN);

    applyRadarSettings();
    myCam.init();        
    network.init();      
    
    if (MDNS.begin("safebaige")) { 
        MDNS.addService("http", "tcp", 80);
        ui.updateMessage("READY", ST77XX_GREEN);
    } else {
        ui.updateMessage("MDNS:ERR", ST77XX_RED);
    }
    
    // FORCE INITIAL RENDER so road appears immediately
    ui.render(0, nullptr); 
    
    lastValidRadarTime = millis();
}

void loop() {
    int newTargets = RadarParser::parse(Serial1, activeTargets, 5, rawDebugBuffer, &rawDebugLen);

    if (newTargets > 0) {
        globalTargetCount = newTargets;
        lastValidRadarTime = millis();
        
        if (trackingStartTime == 0) {
            trackingStartTime = millis();
        } else if ((millis() - trackingStartTime) > cfg_cam_trigger_ms) {
            network.forceStartStream(); 
        }

        for (int i = 0; i < newTargets; i++) {
            activeTargets[i].smoothedDist = distFilter.smooth(i, (float)activeTargets[i].distance);
        }
        ui.render(newTargets, activeTargets);
        
    } else {
        if (debugMode && Serial1.available() > 0) {
            rawDebugLen = 0;
            while(Serial1.available() > 0 && rawDebugLen < 64) {
                rawDebugBuffer[rawDebugLen++] = Serial1.read();
            }
        }

        if (millis() - lastValidRadarTime > DATA_PERSIST_MS) {
            trackingStartTime = 0; 
            if (globalTargetCount > 0) {
                globalTargetCount = 0;
                for (int i = 0; i < 5; i++) distFilter.reset(i);
                ui.render(0, nullptr); // This ensures road stays visible even with 0 targets
            }
        }
    }
    
    // Optimization: If the screen is blank/idle, periodically redraw the road 
    // to ensure it hasn't been wiped by a glitch
    static unsigned long lastIdleRefresh = 0;
    if (globalTargetCount == 0 && (millis() - lastIdleRefresh > 2000)) {
        ui.render(0, nullptr);
        lastIdleRefresh = millis();
    }

    delay(5);
}