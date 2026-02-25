#ifndef NETWORK_MANAGER_H
#define NETWORK_MANAGER_H

#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include "esp_camera.h" 
#include "LD2451_Defines.h"

// --- Variables from main.cpp ---
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
extern uint8_t cfg_rapid_threshold;     
extern uint32_t cfg_linger_threshold_ms; 

extern void applyRadarSettings(); 

class NetworkManager {
private:
    AsyncWebServer _server;

    // The Dashboard UI
    const char* index_html = R"rawliteral(
<!DOCTYPE html><html><head><title>SafeBaige Pro</title>
<meta name="viewport" content="width=device-width, initial-scale=1">
<style>
    body { font-family: -apple-system, sans-serif; background: #121212; color: #eee; margin: 0; padding: 20px; }
    .container { max-width: 500px; margin: auto; }
    .card { background: #1e1e1e; padding: 15px; border-radius: 12px; margin-bottom: 20px; border: 1px solid #333; }
    .group-title { color: #00cfcf; font-size: 0.9em; font-weight: bold; margin-bottom: 15px; text-transform: uppercase; border-bottom: 1px solid #333; padding-bottom: 5px;}
    .row { display: flex; align-items: center; justify-content: space-between; margin-bottom: 12px; }
    .row label { flex: 1; font-size: 0.85em; }
    .row input[type=range] { flex: 1.5; margin: 0 10px; }
    .val { width: 45px; text-align: right; font-family: monospace; color: #00cfcf; }
    select, button { background: #333; color: white; border: 1px solid #444; padding: 8px; border-radius: 4px; }
    button { background: #00cfcf; color: black; font-weight: bold; width: 100%; cursor: pointer; border: none; margin-top: 10px; }
    button:active { opacity: 0.8; }
    .stream { width: 100%; border-radius: 8px; margin-bottom: 10px; background: #000; }
</style>
</head><body>
    <div class="container">
        <img src="http://safebaige.local:81/stream" class="stream">
        
        <div class="card">
            <div class="group-title">Camera & View</div>
            <div class="row">
                <button style="width:48%; margin:0" onclick="fetch('/cam?cmd=flip')">Vertical Flip</button>
                <button style="width:48%; margin:0" onclick="fetch('/cam?cmd=mirror')">Mirror</button>
            </div>
        </div>

        <div class="card">
            <div class="group-title">Radar Hardware (LD2451)</div>
            
            <div class="row">
                <label>Max Dist</label>
                <input type="range" id="dist" min="1" max="100" value="10" oninput="v('vd',this.value)">
                <span class="val"><span id="vd">10</span>m</span>
            </div>

            <div class="row">
                <label>Min Speed</label>
                <input type="range" id="speed" min="0" max="120" value="0" oninput="v('vs',this.value)">
                <span class="val"><span id="vs">0</span>km/h</span>
            </div>

            <div class="row">
                <label>Direction</label>
                <select id="dir" style="width: 60%;">
                    <option value="2">Both Directions</option>
                    <option value="1">Approaching Only</option>
                    <option value="0">Moving Away Only</option>
                </select>
            </div>

            <div class="row">
                <label>Delay Time</label>
                <input type="range" id="delay" min="0" max="255" value="0" oninput="v('vy',this.value)">
                <span class="val"><span id="vy">0</span>s</span>
            </div>

            <div class="row">
                <label>Sensitivity (SNR)</label>
                <input type="range" id="snr" min="0" max="100" value="10" oninput="v('vn',this.value)">
                <span class="val"><span id="vn">10</span></span>
            </div>

            <div class="row">
                <label>Trigger Count</label>
                <input type="range" id="acc" min="1" max="20" value="1" oninput="v('va',this.value)">
                <span class="val"><span id="va">1</span>f</span>
            </div>
        </div>

        <div class="card">
            <div class="group-title">System Alert Logic</div>
            <div class="row">
                <label>Rapid Speed</label>
                <input type="range" id="rapid" min="5" max="150" value="15" oninput="v('vr',this.value)">
                <span class="val"><span id="vr">15</span>km/h</span>
            </div>
            <div class="row">
                <label>Linger Time</label>
                <input type="range" id="linger" min="500" max="10000" step="500" value="3000" oninput="v('vl',this.value)">
                <span class="val"><span id="vl">3000</span>ms</span>
            </div>
            <button onclick="save()">APPLY ALL SETTINGS</button>
        </div>
    </div>

    <script>
        function v(id,val){ document.getElementById(id).innerText = val; }
        function save() {
            const p = `dist=${g('dist')}&dir=${g('dir')}&speed=${g('speed')}&delay=${g('delay')}&acc=${g('acc')}&snr=${g('snr')}&rapid=${g('rapid')}&linger=${g('linger')}`;
            fetch('/config?'+p).then(()=>alert('Settings Synced'));
        }
        function g(id){ return document.getElementById(id).value; }
    </script>
</body></html>)rawliteral";

public:
    NetworkManager() : _server(80) {}

    void init() {
        WiFi.softAP("SafeBaige", "drivesafe");

        // --- Dashboard ---
        _server.on("/", HTTP_GET, [this](AsyncWebServerRequest *request){
            request->send(200, "text/html", index_html);
        });

        // --- Camera Flip/Mirror ---
        _server.on("/cam", HTTP_GET, [](AsyncWebServerRequest *request){
            sensor_t * s = esp_camera_sensor_get();
            if (request->hasParam("cmd")) {
                String cmd = request->getParam("cmd")->value();
                if(cmd == "flip") s->set_vflip(s, !s->status.vflip);
                if(cmd == "mirror") s->set_hmirror(s, !s->status.hmirror);
            }
            request->send(200, "text/plain", "OK");
        });

        // --- Expanded Radar Configuration ---
        _server.on("/config", HTTP_GET, [](AsyncWebServerRequest *request){
            // Radar Hardware Params
            if (request->hasParam("dist"))  cfg_max_dist = request->getParam("dist")->value().toInt();
            if (request->hasParam("dir"))   cfg_direction = request->getParam("dir")->value().toInt();
            if (request->hasParam("speed")) cfg_min_speed = request->getParam("speed")->value().toInt();
            if (request->hasParam("delay")) cfg_delay_time = request->getParam("delay")->value().toInt();
            if (request->hasParam("acc"))   cfg_trigger_acc = request->getParam("acc")->value().toInt();
            if (request->hasParam("snr"))   cfg_snr_limit = request->getParam("snr")->value().toInt();
            
            // System Logic Params
            if (request->hasParam("rapid"))  cfg_rapid_threshold = request->getParam("rapid")->value().toInt();
            if (request->hasParam("linger")) cfg_linger_threshold_ms = request->getParam("linger")->value().toInt();
            
            applyRadarSettings(); // Re-push to LD2451 via UART
            request->send(200, "text/plain", "OK");
        });

        // --- Debug and Data endpoints remain as they were ---
        
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