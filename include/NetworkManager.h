#ifndef NETWORK_MANAGER_H
#define NETWORK_MANAGER_H

#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include "DisplayModule.h"
#include "RadarConfig.h"


extern DisplayModule ui;

extern volatile int globalTargetCount;
extern bool yoloVetoActive; // Global flag to kill alerts
extern volatile float lastVetoDistance;
extern RadarTarget activeTargets[];
extern bool pendingConfigChange;
extern uint8_t nextRange, nextDir, nextMinSpd, nextSens;


class NetworkManager {
private:
    AsyncWebServer _server;

    //Helper to send commands to radar (if needed in future)
    void sendRadarCommand(uint8_t* cmd, size_t len) {
        Serial2.write(cmd, len);
        delay(60); // Small delay to ensure radar has time to process command before next one arrives
    }

public:
    NetworkManager() : _server(80) {}

    void init() {
        WiFi.softAP("SafeBaige", "drivesafe");

        _server.on("/yolo_feedback", HTTP_GET, [](AsyncWebServerRequest *request){
            if (request->hasParam("detected")) {
                //yoloVetoActive = (request->getParam("detected")->value() == "0"); 
                //ui.updateMessage(yoloVetoActive ? "VETO: ACTIVE" : "VETO: OFF", ST77XX_YELLOW);
            }
            request->send(200, "text/plain", "ACK");
        });
        
        // Data endpoint for debugging/monitoring (returns JSON with current targets)
        //_server.on("/data", HTTP_GET, [](AsyncWebServerRequest *request){
        //    int countSnapshot = globalTargetCount;
        //    
        //    String json;
        //    json.reserve(256); 
        //    
        //    json = "{\"status\":\"online\",\"count\":";
        //    json += String(countSnapshot);
        //    json += ",\"targets\":[";
        //    
        //    for(int i = 0; i < countSnapshot; i++) {
        //        json += "{\"id\":";
        //        json += String(i);
        //        json += ",\"dist\":";
        //        json += String(activeTargets[i].distance);
        //        json += "}";
        //        if(i < countSnapshot - 1) json += ",";
        //    }
        //    json += "]}";
        //    
        //    // Use 'application/json' to help curl/apps parse it
        //    request->send(200, "application/json", json);
        //});

        // Configuration endpoint to set radar parameters (range, direction to track (approaching/receding), sensitivity, min speed)
        //_server.on("/config", HTTP_GET, [](AsyncWebServerRequest *request){
            //nextRange = request->hasParam("range") ? request->getParam("range")->value().toInt() : 100;
            // ... capture other params ...
            //pendingConfigChange = true; 
            
            //ui.updateMessage("CFG: QUEUED", ST77XX_MAGENTA);
            //request->send(200, "text/plain", "OK");
        //});

        //_server.on("/profile", HTTP_GET, [](AsyncWebServerRequest *request){
        //    if (request->hasParam("side")) {
        //        String side = request->getParam("side")->value();
        //        currentTrafficSide = (side == "left") ? LEFT_HAND_DRIVE : RIGHT_HAND_DRIVE;
        //    }
        //    if (request->hasParam("mode")) {
        //        String mode = request->getParam("mode")->value();
        //        if (mode == "city") {
        //            nextRange = 30; nextMinSpd = 10; nextSens = 3;
        //        } else if (mode == "highway") {
        //            nextRange = 100; nextMinSpd = 5; nextSens = 8;
        //        }
        //        pendingConfigChange = true;
        //    }
        //    request->send(200, "text/plain", "Profile Applied");
        //});


        _server.begin();
    }

};

#endif