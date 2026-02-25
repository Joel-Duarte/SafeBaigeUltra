#ifndef NETWORK_MANAGER_H
#define NETWORK_MANAGER_H

#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include "LD2451_Defines.h"

// --- Variables and Functions from main.cpp ---
extern bool yoloVetoActive;
extern int globalTargetCount;
extern RadarTarget activeTargets[];
extern bool debugMode;
extern int rawDebugLen;
extern uint8_t rawDebugBuffer[];

// Radar Configuration Globals
extern uint8_t cfg_max_dist;
extern uint8_t cfg_direction;
extern uint8_t cfg_min_speed;
extern uint8_t cfg_delay_time;  
extern uint8_t cfg_trigger_acc;  
extern uint8_t cfg_snr_limit; 

// The function in main.cpp that talks to the hardware
extern void applyRadarSettings(); 

class NetworkManager {
private:
    AsyncWebServer _server;

public:
    NetworkManager() : _server(80) {}

    void init() {
        // Start Access Point
        WiFi.softAP("SafeBaige", "drivesafe");

        // --- YOLO Veto Endpoint ---
        _server.on("/yolo_feedback", HTTP_GET, [](AsyncWebServerRequest *request){
            if (request->hasParam("detected")) {
                yoloVetoActive = (request->getParam("detected")->value() == "1"); 
            }
            request->send(200, "text/plain", "OK");
        });

        // --- Radar Configuration Endpoint ---
        // Usage: http://safebaige.local/config?dist=15&speed=5&snr=10
        _server.on("/config", HTTP_GET, [](AsyncWebServerRequest *request){
            if (request->hasParam("dist"))  cfg_max_dist    = (uint8_t)request->getParam("dist")->value().toInt();
            if (request->hasParam("dir"))   cfg_direction   = (uint8_t)request->getParam("dir")->value().toInt();
            if (request->hasParam("speed")) cfg_min_speed   = (uint8_t)request->getParam("speed")->value().toInt();
            if (request->hasParam("delay")) cfg_delay_time  = (uint8_t)request->getParam("delay")->value().toInt();
            if (request->hasParam("acc"))   cfg_trigger_acc = (uint8_t)request->getParam("acc")->value().toInt();
            if (request->hasParam("snr"))   cfg_snr_limit   = (uint8_t)request->getParam("snr")->value().toInt();
            
            // Push updated globals to the physical radar sensor
            applyRadarSettings(); 
            
            request->send(200, "text/plain", "Radar Settings Updated & Applied");
        });
        
        // --- JSON Data Endpoint for external dashboards ---
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
        
        // --- Debug Text Endpoint ---
        _server.on("/debug", HTTP_GET, [](AsyncWebServerRequest *request){
            if (!debugMode) {
                request->send(403, "text/plain", "Debug mode is disabled.");
                return;
            }

            String output = "SAFEBAIGE RADAR SYSTEM DEBUG\n";
            output += "=============================\n";
            output += "CURRENT CONFIG:\n";
            output += " Max Distance:  " + String(cfg_max_dist) + "m\n";
            output += " Min Speed:     " + String(cfg_min_speed) + "km/h\n";
            output += " Direction:     " + String(cfg_direction) + " (0:Away, 1:App, 2:Both)\n";
            output += " SNR Limit:     " + String(cfg_snr_limit) + "\n";
            output += " Trigger Acc:   " + String(cfg_trigger_acc) + "\n";
            output += "-----------------------------\n";
            output += "LIVE STATUS:\n";
            output += " Targets Found: " + String(globalTargetCount) + "\n";
            output += " YOLO Veto:     " + String(yoloVetoActive ? "ACTIVE" : "INACTIVE") + "\n";
            output += "\nRAW HEX BUFFER:\n";
            
            if (rawDebugLen == 0) {
                output += "Empty";
            } else {
                for (int i = 0; i < rawDebugLen; i++) {
                    char hex[4];
                    sprintf(hex, "%02X ", rawDebugBuffer[i]);
                    output += hex;
                }
            }
            
            request->send(200, "text/plain", output);
        });

        _server.begin();
    }
};

#endif