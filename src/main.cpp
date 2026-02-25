#include <Arduino.h>
#include <WiFi.h>
#include <ESPmDNS.h>
#include "Camera.h"
#include "StreamServer.h"
#include "DisplayModule.h"

//#include "../include/RadarConfig.h"
//#include "../include/RadarParser.h"
//#include "../include/SafetySystems.h"
//#include "../include/NetworkManager.h"
//#include "../include/FilterModule.h" 

//SignalFilter radarFilter;


//RadarTarget activeTargets[5];
//SafetySystems safety;
DisplayModule ui;
//NetworkManager network;
//volatile int globalTargetCount = 0;
//unsigned long lastValidRadarTime = 0;
//const int DATA_PERSIST_MS = 250;
//bool yoloVetoActive = false;
//volatile float lastVetoDistance = 0.0f;
//bool pendingConfigChange = false;
//uint8_t nextRange, nextDir, nextMinSpd, nextSens;
//
//TrafficSide currentTrafficSide = RIGHT_HAND_DRIVE;


//unsigned long lastCarSeenTime = 0;
//const int CLEAR_TIMEOUT = 500;
//bool alreadyClear = false;
Camera myCam;

void setup() {
    Serial.begin(115200);
    //Serial2.begin(115200, SERIAL_8N1, RAD_RX, RAD_TX);
    myCam.init();
    //safety.init();
    ui.init();
    //network.init();
    WiFi.softAP("SafeBaige", "drivesafe");

    if (!MDNS.begin("safebaige")) { 
        Serial.println("Error setting up MDNS responder!");
    } else {
        Serial.println("mDNS responder started: http://safebaige.local");
        // Add service to MDNS for HTTP on port 81
        MDNS.addService("http", "tcp", 81);
    }

    // 3. Start the Library service
    startCameraServer();
    Serial.println("Safebaige Modular Boot Complete");
}

void loop() {
  delay(1000);
}
