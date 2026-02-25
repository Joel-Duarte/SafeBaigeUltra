#ifndef NETWORK_MANAGER_H
#define NETWORK_MANAGER_H

#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include "LD2451_Defines.h"

extern bool yoloVetoActive;
extern int globalTargetCount;
extern RadarTarget activeTargets[];

// References to the config variables in main.cpp
extern uint8_t cfg_max_dist;
extern uint8_t cfg_min_speed;
extern uint8_t cfg_direction;
extern uint8_t cfg_delay_time;  
extern uint8_t cfg_sensitivity; 
extern uint8_t cfg_trigger_acc;  
extern uint8_t cfg_snr_limit; 
extern void applyRadarSettings(); 

class NetworkManager {
private:
    AsyncWebServer _server;

public:
    NetworkManager() : _server(80) {}

    void init() {
        WiFi.softAP("SafeBaige", "drivesafe");

        // YOLO Veto Toggle
        _server.on("/yolo_feedback", HTTP_GET, [](AsyncWebServerRequest *request){
            if (request->hasParam("detected")) {
                yoloVetoActive = (request->getParam("detected")->value() == "1"); 
            }
            request->send(200, "text/plain", "OK");
        });

        // Dynamic Radar Configuration
        _server.on("/config", HTTP_GET, [](AsyncWebServerRequest *request){
            if (request->hasParam("dist"))  cfg_max_dist    = request->getParam("dist")->value().toInt();
            if (request->hasParam("speed")) cfg_min_speed   = request->getParam("speed")->value().toInt();
            if (request->hasParam("dir"))   cfg_direction   = request->getParam("dir")->value().toInt();
            if (request->hasParam("delay")) cfg_delay_time  = request->getParam("delay")->value().toInt();
            if (request->hasParam("acc"))   cfg_trigger_acc = request->getParam("acc")->value().toInt();
            if (request->hasParam("snr"))   cfg_snr_limit   = request->getParam("snr")->value().toInt();
            
            applyRadarSettings(); 
            request->send(200, "text/plain", "Settings Applied");
        });
        
        _server.on("/data", HTTP_GET, [](AsyncWebServerRequest *request){
            String json = "{\"count\":" + String(globalTargetCount) + ",\"targets\":[";
            for(int i = 0; i < globalTargetCount; i++) {
                json += "{\"d\":" + String(activeTargets[i].distance) + 
                        ",\"s\":" + String(activeTargets[i].speed) + 
                        ",\"app\":" + String(activeTargets[i].approaching ? 1 : 0) + "}";
                if(i < globalTargetCount - 1) json += ",";
            }
            json += "]}";
            request->send(200, "application/json", json);
        });

        _server.begin();
    }
};

#endif