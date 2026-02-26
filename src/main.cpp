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
uint8_t cfg_max_dist    = 40;//  1-100 (10 as min is recommended) meters
uint8_t cfg_direction   = 2;// 0: Away, 1: Approach, 2: Both
uint8_t cfg_min_speed   = 0;//min speed to detect ()
uint8_t cfg_delay_time  = 0;  // 0 or 1 since safebaige has persistence timeout
uint8_t cfg_trigger_acc = 1;  // 3/4 Required consecutive detections before reporting
uint8_t cfg_snr_limit   = 0;   //0 - 255 4 is the default / use 6–10: If radar is giving "ghost" detections
uint8_t cfg_rapid_threshold = 15; // speed that cars need to be approaching at to be considered BAD and rendered red

uint32_t cameraTimerMs = 15000;  // ammount of seconds to keep camera alive 
// --- Hardware & System Configuration ---

const int RADAR_TX_PIN = 1;
const int RADAR_RX_PIN = 2;
const int DATA_PERSIST_MS = 800;// Time to hold target on screen after motion stops
// --- Global Variables (Accessed by NetworkManager/Webhooks) ---

uint8_t rawDebugBuffer[64];
int rawDebugLen = 0;
bool debugMode = true;

int globalTargetCount = 0;
bool yoloVetoActive = false;

unsigned long carFirstDetectedTime = 0;
bool radarUpdatePending = false;

RadarTarget activeTargets[5];
unsigned long lastValidRadarTime = 0;
struct RadarSnapshot {
    int count;
    int distance[5];
    int speed[5];
    bool approaching[5];
};
// --- Module Instances ---

NetworkManager network;
DisplayModule ui;
Camera myCam;
SignalFilter distFilter;

void applyRadarSettings() {
    // 1. Send configuration block (Enable -> Set Params -> End)

    RadarConfig::sendDefaults(
        Serial1,
        cfg_max_dist,
        cfg_direction,
        cfg_min_speed,
        cfg_delay_time,
        cfg_trigger_acc,
        cfg_snr_limit
    );
    delay(100);
    // 2. Explicit "Start Reporting" command to ensure data flow

    uint8_t startCmd[] = {
        0xFD,0xFC,0xFB,0xFA,
        0x04,0x00,
        0x62,0x00,
        0x04,0x03,0x02,0x01
    };

    Serial1.write(startCmd, sizeof(startCmd));
    Serial1.flush();
}

void updateCameraPower() {

    static bool cameraSleeping = false;
    unsigned long now = millis();

    if (now - lastValidRadarTime > cameraTimerMs) {

        if (!cameraSleeping) {
            enterLowPowerMode();
            cameraSleeping = true;
        }
    }
    else {

        if (cameraSleeping) {
            exitLowPowerMode();
            cameraSleeping = false;
        }
    }
}

void setup() {
    Serial1.begin(115200, SERIAL_8N1, RADAR_TX_PIN, RADAR_RX_PIN);
    delay(500);

    ui.init();
    ui.updateMessage("BOOTING", ST77XX_CYAN);

    applyRadarSettings();

    myCam.init();
    network.init();
    startCameraServer();

    if (MDNS.begin("safebaige")) {
        MDNS.addService("http", "tcp", 80);
        ui.updateMessage("READY", ST77XX_GREEN);
    } else {
        ui.updateMessage("MDNS:ERR", ST77XX_RED);
    }
    lastValidRadarTime = millis();
}

void loop() {
    static RadarSnapshot lastSnapshot = { -1 };
    static unsigned long lastForcedSend = 0;
    const unsigned long FORCE_INTERVAL_MS = 1000;
    bool shouldSend = false;
    // 1. Parse Radar Frame
    int newTargets = RadarParser::parse(
        Serial1,
        activeTargets,
        5,
        rawDebugBuffer,
        &rawDebugLen
    );
    // 2. Apply Config if Needed
    if (radarUpdatePending) {
        radarUpdatePending = false;
        applyRadarSettings();
        ui.redrawBackground();
    }
    // 3. Valid Targets Detected
    if (newTargets > 0) {
        if (carFirstDetectedTime == 0)
            carFirstDetectedTime = millis();

        globalTargetCount = newTargets;
        lastValidRadarTime = millis();

        if (isLowPower())
            exitLowPowerMode();

        for (int i = 0; i < newTargets; i++) {
            activeTargets[i].smoothedDist =
                distFilter.smooth(i, (float)activeTargets[i].distance);
        }
        ui.render(newTargets, activeTargets);
        if (globalTargetCount != lastSnapshot.count) {
            shouldSend = true;
        }
        else {
            for (int i = 0; i < globalTargetCount; i++) {
                if (abs(activeTargets[i].distance - lastSnapshot.distance[i]) > 2) {
                    shouldSend = true;
                    break;
                }
                if (abs(activeTargets[i].speed - lastSnapshot.speed[i]) > 1) {
                    shouldSend = true;
                    break;
                }
                if (activeTargets[i].approaching != lastSnapshot.approaching[i]) {
                    shouldSend = true;
                    break;
                }
            }
        }
        // Websocket heartbeat
        if (!shouldSend &&
            globalTargetCount > 0 &&
            millis() - lastForcedSend > FORCE_INTERVAL_MS) {
            shouldSend = true;
        }
        // Websocket update with new target
        if (shouldSend) {
            network.sendRadarUpdate();
            lastForcedSend = millis();
            lastSnapshot.count = globalTargetCount;
            for (int i = 0; i < globalTargetCount; i++) {
                lastSnapshot.distance[i] = activeTargets[i].distance;
                lastSnapshot.speed[i] = activeTargets[i].speed;
                lastSnapshot.approaching[i] = activeTargets[i].approaching;
            }
        }
    }
    else {
        // 4. Debug Raw Capture
        if (debugMode && Serial1.available() > 0) {
            rawDebugLen = 0;
            while (Serial1.available() > 0 && rawDebugLen < 64) {
                rawDebugBuffer[rawDebugLen++] = Serial1.read();
            }
        }
        // 5. Persistence Timeout
        if (millis() - lastValidRadarTime > DATA_PERSIST_MS) {
            if (globalTargetCount > 0) {
                globalTargetCount = 0;
                for (int i = 0; i < 5; i++)
                    distFilter.reset(i);
                ui.render(0, nullptr);
                network.sendRadarUpdate();
                lastForcedSend = millis();
                lastSnapshot.count = 0;
            }
            else {
                carFirstDetectedTime = 0;
            }
        }
    }

    updateCameraPower();
    //network.cleanupWS();
    network.handleHeartbeat();
    delay(5);
}