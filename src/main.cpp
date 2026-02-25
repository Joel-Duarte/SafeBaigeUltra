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
uint8_t cfg_max_dist    = 10; // meters
uint8_t cfg_direction   = 2;   // 0: Away, 1: Approach, 2: Both 
uint8_t cfg_min_speed   = 0;   
uint8_t cfg_delay_time  = 0;   
uint8_t cfg_trigger_acc = 1;   
uint8_t cfg_snr_limit   = 0;   

// --- Hardware & System Configuration ---
const int RADAR_TX_PIN = 1; 
const int RADAR_RX_PIN = 2; 
const int DATA_PERSIST_MS = 800; // Time to hold target on screen after motion stops

// --- Global Variables (Accessed by NetworkManager/Webhooks) ---
uint8_t rawDebugBuffer[64];
int rawDebugLen = 0;
bool debugMode = true;
int globalTargetCount = 0;
bool yoloVetoActive = false;
uint8_t cfg_rapid_threshold = 15;     
uint32_t cfg_linger_threshold_ms = 3000; 
unsigned long carFirstDetectedTime = 0;
bool lingerAlertSent = false;

// --- Module Instances ---
NetworkManager network;
DisplayModule ui;
Camera myCam;
SignalFilter distFilter;
RadarTarget activeTargets[5];
unsigned long lastValidRadarTime = 0;

void applyRadarSettings() {
    // 1. Send configuration block (Enable -> Set Params -> End)
    RadarConfig::sendDefaults(Serial1, cfg_max_dist, cfg_direction, cfg_min_speed, cfg_delay_time, cfg_trigger_acc, cfg_snr_limit);
    delay(100);

    // 2. Explicit "Start Reporting" command to ensure data flow
    uint8_t startCmd[] = {0xFD, 0xFC, 0xFB, 0xFA, 0x04, 0x00, 0x62, 0x00, 0x04, 0x03, 0x02, 0x01};
    Serial1.write(startCmd, sizeof(startCmd));
    Serial1.flush();
}

void setup() {
    // Initialize Radar UART
    Serial1.begin(115200, SERIAL_8N1, RADAR_TX_PIN, RADAR_RX_PIN);
    delay(500);

    // Initialize Display
    ui.init();
    ui.updateMessage("BOOTING", ST77XX_CYAN);

    // Sync Radar Hardware
    applyRadarSettings();

    // Initialize Subsystems
    myCam.init();        
    network.init();      
    startCameraServer(); 
    
    // Setup mDNS for http://safebaige.local
    if (MDNS.begin("safebaige")) { 
        MDNS.addService("http", "tcp", 80);
        ui.updateMessage("READY", ST77XX_GREEN);
    } else {
        ui.updateMessage("MDNS:ERR", ST77XX_RED);
    }
    
    lastValidRadarTime = millis();
}

void loop() {
    // 1. Try to parse incoming Radar data
    int newTargets = RadarParser::parse(Serial1, activeTargets, 5, rawDebugBuffer, &rawDebugLen);

    if (newTargets > 0) {
        // --- Linger Logic ---
        if (carFirstDetectedTime == 0) carFirstDetectedTime = millis();
        
        uint32_t elapsed = millis() - carFirstDetectedTime;
        if (elapsed > cfg_linger_threshold_ms && !lingerAlertSent) {
            Serial.println("ALERT: Car lingering detected!"); 
            // Trigger your webhook here
            lingerAlertSent = true;
        }

        globalTargetCount = newTargets;
        lastValidRadarTime = millis();
        // Update smoothing filters and render UI
        for (int i = 0; i < newTargets; i++) {
            activeTargets[i].smoothedDist = distFilter.smooth(i, (float)activeTargets[i].distance);
        }
        ui.render(newTargets, activeTargets);
        
    } else {
        // 2. If no valid frame was parsed, capture raw noise for the debug webhook
        if (debugMode && Serial1.available() > 0) {
            rawDebugLen = 0;
            while(Serial1.available() > 0 && rawDebugLen < 64) {
                rawDebugBuffer[rawDebugLen++] = Serial1.read();
            }
        }

        // 3. Persistence Logic: Fade out cars after a timeout
        if (millis() - lastValidRadarTime > DATA_PERSIST_MS) {
            if (globalTargetCount > 0) {
                globalTargetCount = 0;
                for (int i = 0; i < 5; i++) distFilter.reset(i);
                ui.render(0, nullptr); 
            } else {
                carFirstDetectedTime = 0;
                lingerAlertSent = false;
            }
        }
    }
    
    // Tiny delay to keep the ESP32-S3 Watchdog happy
    delay(5);
}