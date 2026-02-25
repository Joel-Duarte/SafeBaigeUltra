#include <Arduino.h>
#include <WiFi.h>
#include <ESPmDNS.h>
#include "Camera.h"
#include "StreamServer.h"
#include "DisplayModule.h"

DisplayModule ui;
Camera myCam;

void setup() {
    Serial.begin(115200);

    // 1. Initialize Display FIRST so we can see status
    ui.init(); 
    // At this point, the screen shows "BOOTING..." (from the DisplayModule.h we updated)

    // 2. Initialize Camera and capture result
    // We modify the myCam.init() call to return the status string
    String camStatus = myCam.init(); 
    
    if (camStatus == "OK") {
        ui.updateMessage("CAMERA: OK", ST77XX_GREEN);
    } else {
        ui.updateMessage("CAMERA: ERROR", ST77XX_RED);
        // We continue anyway, but the error stays on screen
    }

    delay(500); // Small delay so you can actually read the status
    ui.updateMessage("WIFI: STARTING...", ST77XX_CYAN);

    // 3. Network Setup
    WiFi.softAP("SafeBaige", "drivesafe");

    if (!MDNS.begin("safebaige")) { 
        Serial.println("MDNS Error!");
        ui.updateMessage("MDNS: ERROR", ST77XX_ORANGE);
    } else {
        Serial.println("mDNS started: http://safebaige.local");
        MDNS.addService("http", "tcp", 81);
        ui.updateMessage("SYSTEM: READY", ST77XX_GREEN);
    }

    // 4. Start the Video Stream
    startCameraServer();
    Serial.println("Safebaige Modular Boot Complete");
}

void loop() {
    // You can add a check here to update the UI if the stream is active
    delay(1000);
}