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
extern uint8_t cfg_max_dist;              // 1–100
extern uint8_t cfg_direction;             // 0–2
extern uint8_t cfg_min_speed;             // 0–120
extern uint8_t cfg_delay_time;            // 0–30
extern uint8_t cfg_trigger_acc;           // 1–10
extern uint8_t cfg_snr_limit;             // 0–255
extern uint8_t cfg_rapid_threshold;       // 5–150
extern uint32_t cfg_linger_threshold_ms;  // 500–10000
extern bool radarUpdatePending;
//extern void applyRadarSettings();

class NetworkManager {
private:
    AsyncWebServer _server;

const char* index_html = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
<title>SafeBaige Pro</title>
<meta name="viewport" content="width=device-width, initial-scale=1">
<style>
body { font-family:-apple-system,sans-serif;background:#121212;color:#eee;margin:0;padding:20px;}
.container{max-width:520px;margin:auto;}
.card{background:#1e1e1e;padding:15px;border-radius:12px;margin-bottom:20px;border:1px solid #333;}
.group-title{color:#00cfcf;font-size:0.85em;font-weight:bold;margin-bottom:15px;text-transform:uppercase;border-bottom:1px solid #333;padding-bottom:5px;}
.row{display:flex;align-items:center;justify-content:space-between;margin-bottom:12px;gap:10px;}
.row label{flex:1;font-size:0.9em;}
input,select{flex:1.2;background:#333;color:white;border:1px solid #444;padding:6px;border-radius:6px;}
button{background:#333;color:white;border:1px solid #444;padding:10px;border-radius:6px;cursor:pointer;flex:1;}
button.primary{background:#00cfcf;color:black;border:none;font-weight:bold;margin-top:10px;}
.btn-group{display:flex;gap:10px;margin-bottom:10px;}
.stream-box{width:100%;border-radius:8px;margin-bottom:15px;border:2px solid #333;overflow:hidden;background:#000;}
.stream-box img{width:100%;display:block;}
</style>
</head>

<body>
<div class="container">

<div class="stream-box">
<img src="http://safebaige.local:81/stream">
</div>

<div class="card">
<div class="group-title">Camera</div>
<div class="btn-group">
<button onclick="cmd('flip')">Vertical Flip</button>
<button onclick="cmd('mirror')">Horizontal Mirror</button>
</div>
</div>

<div class="card">
<div class="group-title">Radar Hardware (LD2451)</div>

<div class="row">
    <label>Load Profile</label>
    <select id="profile" onchange="applyProfile(this.value)">
        <option value="">Select Profile</option>
        <option value="city">City Riding</option>
        <option value="highway">Highway/Open</option>
        <option value="rain">Rain / Bad Weather</option>
        <option value="commuter">Commuter</option>
        <option value="performance">Performance</option>
    </select>
</div>

<div class="row">
<label>Max Distance (1-100m)</label>
<input type="number" id="dist" min="1" max="100">
</div>

<div class="row">
<label>Direction</label>
<select id="dir">
<option value="2">Both</option>
<option value="1">Approaching Only</option>
<option value="0">Moving Away Only</option>
</select>
</div>

<div class="row">
<label>Min Speed (0-120km/h)</label>
<input type="number" id="speed" min="0" max="120">
</div>

<div class="row">
<label>Delay Time (0-30s)</label>
<input type="number" id="delay" min="0" max="30">
</div>

<div class="row">
<label>Trigger Accuracy (1-10)</label>
<input type="number" id="acc" min="1" max="10">
</div>

<div class="row">
<label>SNR Limit (0-255 (4 is default))</label>
<input type="number" id="snr" min="0" max="255">
</div>
</div>


<div class="card">
<div class="group-title">System Alert Logic</div>

<div class="row">
<label>Approaching Speed to turn cars RED (km/h)</label>
<input type="number" id="rapid" min="5" max="150">
</div>

<div class="row">
<label>UI Linger (ms)</label>
<input type="number" id="linger" min="500" max="10000" step="500">
</div>

<button class="primary" onclick="save()">SAVE ALL CONFIGS</button>
<button style="margin-top:10px;font-size:0.8em;" onclick="cmd('reboot')">Reboot Device</button>
</div>

</div>

<script>
function g(id){return document.getElementById(id).value;}
function s(id,val){document.getElementById(id).value=val;}

function cmd(type){
fetch(`/cam?cmd=${type}`).then(()=>{if(type!=='reboot')location.reload();});
}

function save(){
const params=new URLSearchParams({
dist:g('dist'),
dir:g('dir'),
speed:g('speed'),
delay:g('delay'),
acc:g('acc'),
snr:g('snr'),
rapid:g('rapid'),
linger:g('linger')
});
fetch('/config?'+params.toString())
.then(r=>r.text())
.then(()=>alert('Settings Applied'));
}

window.onload=function(){
fetch('/config_json')
.then(r=>r.json())
.then(cfg=>{
s('dist',cfg.dist);
s('dir',cfg.dir);
s('speed',cfg.speed);
s('delay',cfg.delay);
s('acc',cfg.acc);
s('snr',cfg.snr);
s('rapid',cfg.rapid);
s('linger',cfg.linger);
});
}
// profile presets
const presets = {
    city:      {dist:40, speed:0, delay:0, acc:5, snr:6, rapid:15, linger:3000},
    highway:   {dist:100, speed:10, delay:0, acc:3, snr:4, rapid:20, linger:3000},
    rain:      {dist:60, speed:5, delay:1, acc:6, snr:10, rapid:15, linger:4000},
    commuter:  {dist:70, speed:5, delay:0, acc:4, snr:4, rapid:15, linger:3000},
    performance:{dist:100, speed:0, delay:0, acc:2, snr:4, rapid:25, linger:2500}
};

function applyProfile(p) {
    if (!presets[p]) return;
    const cfg = presets[p];
    for (const key in cfg) {
        document.getElementById(key).value = cfg[key];
    }
    alert(`Profile "${p}" applied - press SAVE`);
}
</script>

</body>
</html>
)rawliteral";

public:
    NetworkManager() : _server(80) {}

    void init() {
        WiFi.softAP("SafeBaige","drivesafe");

        _server.on("/", HTTP_GET, [this](AsyncWebServerRequest *request){
            request->send(200,"text/html",index_html);
        });

        _server.on("/config_json", HTTP_GET, [](AsyncWebServerRequest *request){
            char json[256];
            snprintf(json, sizeof(json),
                "{\"dist\":%u,"
                "\"dir\":%u,"
                "\"speed\":%u,"
                "\"delay\":%u,"
                "\"acc\":%u,"
                "\"snr\":%u,"
                "\"rapid\":%u,"
                "\"linger\":%lu}",
                cfg_max_dist,
                cfg_direction,
                cfg_min_speed,
                cfg_delay_time,
                cfg_trigger_acc,
                cfg_snr_limit,
                cfg_rapid_threshold,
                cfg_linger_threshold_ms
            );

            request->send(200, "application/json", json);
        });

        _server.on("/cam", HTTP_GET, [](AsyncWebServerRequest *request){
            sensor_t * s = esp_camera_sensor_get();
            if(request->hasParam("cmd")){
                String cmd=request->getParam("cmd")->value();
                if(cmd=="flip") s->set_vflip(s,!s->status.vflip);
                if(cmd=="mirror") s->set_hmirror(s,!s->status.hmirror);
                if(cmd=="reboot") ESP.restart();
            }
            request->send(200,"text/plain","OK");
        });

        _server.on("/config", HTTP_GET, [](AsyncWebServerRequest *request){

            if(request->hasParam("dist"))
                cfg_max_dist = constrain(request->getParam("dist")->value().toInt(),1,100);

            if(request->hasParam("dir"))
                cfg_direction = constrain(request->getParam("dir")->value().toInt(),0,2);

            if(request->hasParam("speed"))
                cfg_min_speed = constrain(request->getParam("speed")->value().toInt(),0,120);

            if(request->hasParam("delay"))
                cfg_delay_time = constrain(request->getParam("delay")->value().toInt(),0,30);

            if(request->hasParam("acc"))
                cfg_trigger_acc = constrain(request->getParam("acc")->value().toInt(),1,10);

            if(request->hasParam("snr"))
                cfg_snr_limit = constrain(request->getParam("snr")->value().toInt(),0,255);

            if(request->hasParam("rapid"))
                cfg_rapid_threshold = constrain(request->getParam("rapid")->value().toInt(),5,150);

            if(request->hasParam("linger"))
                cfg_linger_threshold_ms = constrain(request->getParam("linger")->value().toInt(),500,10000);

            //applyRadarSettings();
            radarUpdatePending = true;
                
            request->send(200,"text/plain","OK");
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