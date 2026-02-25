#include <Arduino.h>
#include <WiFi.h>
#include <ESPmDNS.h>
#include "Camera.h"
#include "StreamServer.h"
#include "DisplayModule.h"
#include "NetworkManager.h"
#include <ESPmDNS.h>

NetworkManager network;
DisplayModule ui;
Camera myCam;

void setup() {
    ui.init();
    myCam.init();      // Starts camera hardware
    network.init();    // Starts Port 80 (Data/Settings)
    startCameraServer(); // Starts Port 81 (Video Stream)
    
    if (!MDNS.begin("safebaige")) { 
        ui.updateMessage("MDNS: ERROR", ST77XX_RED);
    } else {
        MDNS.addService("http", "tcp", 80);
        MDNS.addService("http", "tcp", 81);
        
        ui.updateMessage("MDNS: OK", ST77XX_GREEN);
        Serial.println("mDNS: http://safebaige.local");
    }
    ui.updateMessage("SYSTEM: READY", ST77XX_GREEN);
}

void loop() {
    delay(1000);
}