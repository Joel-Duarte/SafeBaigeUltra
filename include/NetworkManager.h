#ifndef NETWORK_MANAGER_H
#define NETWORK_MANAGER_H

#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include "LD2451_Defines.h"
#include "Camera.h"

extern bool yoloVetoActive;
extern int globalTargetCount;
extern RadarTarget activeTargets[];
extern uint8_t cfg_max_dist, cfg_direction, cfg_min_speed, cfg_delay_time, cfg_trigger_acc, cfg_snr_limit;
extern uint16_t cfg_cam_trigger_ms;

extern void applyRadarSettings(); 
extern void startCameraServer(); 
extern Camera myCam; 

class NetworkManager {
private:
    AsyncWebServer _server;
    bool _isStreaming = false;

public:
    NetworkManager() : _server(80) {}

    void init() {
        WiFi.softAP("SafeBaige", "drivesafe");

        _server.on("/", HTTP_GET, [this](AsyncWebServerRequest *request){
            String html = "<!DOCTYPE html><html><head>";
            html += "<title>SafeBaige OS</title>";
            html += "<meta name='viewport' content='width=device-width, initial-scale=1'>";
            html += "<style>";
            html += "body { font-family: system-ui, sans-serif; background: #0a0a0a; color: #eee; padding: 15px; margin:0; }";
            html += ".card { background: #181818; padding: 20px; border-radius: 15px; margin-bottom: 20px; border: 1px solid #222; }";
            html += "h2 { color: #00cfcf; margin: 0 0 15px 0; font-size: 1.1rem; text-transform: uppercase; }";
            html += "label { display: block; margin: 12px 0 5px; font-size: 0.85rem; color: #888; }";
            html += "input[type=range] { width: 100%; accent-color: #00cfcf; margin-top:10px; }";
            html += "select, input[type=number] { background: #222; border: 1px solid #444; color: white; padding: 8px; border-radius: 6px; width: 100%; box-sizing: border-box; }";
            html += ".btn { background: #00cfcf; color: #000; border: none; padding: 14px; border-radius: 8px; width: 100%; font-weight: bold; cursor: pointer; margin-top: 15px; }";
            html += ".btn-alt { background: #222; color: #00cfcf; border: 1px solid #00cfcf; margin-top: 0; }";
            html += ".stream-box { width: 100%; background: #000; aspect-ratio: 4/3; margin-top: 15px; border-radius: 8px; display: flex; align-items: center; justify-content: center; overflow:hidden; border: 1px solid #333; }";
            html += "output { color: #00cfcf; font-weight: bold; float: right; }";
            html += "</style></head><body>";

            html += "<h1 style='text-align:center; color:#00cfcf;'>SAFEBAIGE <span style='color:#fff;'>OS</span></h1>";

            // --- Live Video Section ---
            html += "<div class='card'><h2>Live Viewfinder</h2>";
            html += "<div class='stream-box' id='stream-container'>";
            if (_isStreaming) {
                // Fixed: Using JavaScript to build the IP so port 81 works correctly
                html += "<img id='stream-img' src='' style='width:100%'>";
                html += "<script>document.getElementById('stream-img').src = 'http://' + window.location.hostname + ':81/stream';</script>";
            } else {
                html += "<p style='color:#555'>Stream Offline (Auto-Trigger Armed)</p>";
            }
            html += "</div>";
            
            html += "<div style='display:grid; grid-template-columns: 1fr 1fr; gap:10px; margin-top:15px;'>";
            html += "<button class='btn' onclick=\"location.href='/stream_toggle'\">" + String(_isStreaming ? "STOP STREAM" : "START STREAM") + "</button>";
            html += "<button class='btn btn-alt' onclick=\"fetch('/cam?cmd=flip')\">FLIP V</button>";
            html += "<button class='btn btn-alt' onclick=\"fetch('/cam?cmd=mirror')\">MIRROR H</button>";
            html += "</div></div>";

            // --- Master Config Section (Restored Missing Settings) ---
            html += "<div class='card'><h2>Master Logic Config</h2>";
            html += "<form action='/config' method='GET'>";
            
            // Activation Delay
            html += "<label>Cam Activation Delay<output id='out_cam'>"+String(cfg_cam_trigger_ms)+"ms</output></label>";
            html += "<input type='range' name='cam_ms' min='0' max='10000' step='500' value='"+String(cfg_cam_trigger_ms)+"' oninput='document.getElementById(\"out_cam\").value = this.value + \"ms\"'>";

            // Radar Max Distance
            html += "<label>Radar Range<output id='out_dist'>"+String(cfg_max_dist)+"m</output></label>";
            html += "<input type='range' name='dist' min='1' max='100' value='"+String(cfg_max_dist)+"' oninput='document.getElementById(\"out_dist\").value = this.value + \"m\"'>";

            // Direction Selection (Restored)
            html += "<label>Direction Tracking</label>";
            html += "<select name='dir'>";
            html += "<option value='0' "+String(cfg_direction==0?"selected":"")+">Moving Away</option>";
            html += "<option value='1' "+String(cfg_direction==1?"selected":"")+">Approaching</option>";
            html += "<option value='2' "+String(cfg_direction==2?"selected":"")+">Both Directions</option>";
            html += "</select>";

            // Minimum Speed (Restored)
            html += "<label>Min Speed Threshold (km/h)</label>";
            html += "<input type='number' name='speed' value='"+String(cfg_min_speed)+"'>";

            // SNR Sensitivity
            html += "<label>Radar Sensitivity (SNR)<output id='out_snr'>"+String(cfg_snr_limit)+"</output></label>";
            html += "<input type='range' name='snr' min='0' max='64' value='"+String(cfg_snr_limit)+"' oninput='document.getElementById(\"out_snr\").value = this.value'>";

            // Confidence Accumulation
            html += "<label>Trigger Confidence<output id='out_acc'>"+String(cfg_trigger_acc)+" frames</output></label>";
            html += "<input type='range' name='acc' min='1' max='10' value='"+String(cfg_trigger_acc)+"' oninput='document.getElementById(\"out_acc\").value = this.value + \" frames\"'>";

            html += "<button type='submit' class='btn'>Apply & Sync Hardware</button>";
            html += "</form></div>";

            html += "</body></html>";
            request->send(200, "text/html", html);
        });

        // --- Handlers ---
        _server.on("/stream_toggle", HTTP_GET, [this](AsyncWebServerRequest *request){
            _isStreaming = !_isStreaming;
            if (_isStreaming) startCameraServer();
            request->redirect("/");
        });

        _server.on("/config", HTTP_GET, [](AsyncWebServerRequest *request){
            if (request->hasParam("cam_ms")) cfg_cam_trigger_ms = request->getParam("cam_ms")->value().toInt();
            if (request->hasParam("dist"))   cfg_max_dist       = (uint8_t)request->getParam("dist")->value().toInt();
            if (request->hasParam("dir"))    cfg_direction      = (uint8_t)request->getParam("dir")->value().toInt();
            if (request->hasParam("speed"))  cfg_min_speed      = (uint8_t)request->getParam("speed")->value().toInt();
            if (request->hasParam("acc"))    cfg_trigger_acc    = (uint8_t)request->getParam("acc")->value().toInt();
            if (request->hasParam("snr"))    cfg_snr_limit      = (uint8_t)request->getParam("snr")->value().toInt();
            
            applyRadarSettings(); 
            request->redirect("/");
        });

        _server.on("/cam", HTTP_GET, [](AsyncWebServerRequest *request){
            if (request->hasParam("cmd")) {
                String cmd = request->getParam("cmd")->value();
                if (cmd == "flip") myCam.toggleFlip();
                if (cmd == "mirror") myCam.toggleMirror();
            }
            request->send(200, "text/plain", "OK");
        });

        _server.begin();
    }

    void forceStartStream() {
        if (!_isStreaming) {
            _isStreaming = true;
            startCameraServer();
        }
    }
};

#endif