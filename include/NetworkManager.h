#ifndef NETWORK_MANAGER_H
#define NETWORK_MANAGER_H

#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include "esp_camera.h"
#include "LD2451_Defines.h"
#include "StreamServer.h"

// -------- EXTERNALS FROM MAIN --------
extern bool yoloVetoActive;
extern int globalTargetCount;
extern RadarTarget activeTargets[];
extern bool debugMode;
extern int rawDebugLen;
extern uint8_t rawDebugBuffer[];
extern unsigned long lastValidRadarTime;
extern uint32_t cameraTimerMs;

// Radar Config
extern uint8_t cfg_max_dist;
extern uint8_t cfg_direction;
extern uint8_t cfg_min_speed;
extern uint8_t cfg_delay_time;
extern uint8_t cfg_trigger_acc;
extern uint8_t cfg_snr_limit;
extern uint8_t cfg_rapid_threshold;
extern bool radarUpdatePending;
extern void exitLowPowerMode();

class NetworkManager {
private:
    AsyncWebServer _server;
    AsyncWebSocket _ws;

    // Fixed JSON buffer (no heap fragmentation)
    char radarJson[1024];

public:
    NetworkManager() 
        : _server(80),
          _ws("/ws") {}

    // --------------------------------------------------------
    void init() {
        WiFi.softAP("SafeBaige","drivesafe");

        // ------------------ WebSocket ------------------
        _ws.onEvent([this](AsyncWebSocket * server,AsyncWebSocketClient * client,AwsEventType type,void * arg,uint8_t *data,size_t len){

            if(type == WS_EVT_CONNECT) {
                Serial.println("WS Client connected");
            }
            else if(type == WS_EVT_DISCONNECT) {
                Serial.println("WS Client disconnected");
            }
        });

        _server.addHandler(&_ws);

        // ------------------ ROOT ------------------
        _server.on("/", HTTP_GET, [this](AsyncWebServerRequest *request){
            request->send(200,"text/html",index_html);
        });

        // ------------------ CONFIG GET ------------------
        _server.on("/config", HTTP_GET, [](AsyncWebServerRequest *request){

            char json[256];

            snprintf(json, sizeof(json),
                "{"
                "\"dist\":%u,"
                "\"dir\":%u,"
                "\"speed\":%u,"
                "\"delay\":%u,"
                "\"acc\":%u,"
                "\"snr\":%u,"
                "\"rapid\":%u,"
                "\"camTimer\":%lu"
                "}",
                cfg_max_dist,
                cfg_direction,
                cfg_min_speed,
                cfg_delay_time,
                cfg_trigger_acc,
                cfg_snr_limit,
                cfg_rapid_threshold,
                cameraTimerMs
            );

            request->send(200, "application/json", json);
        });

        // ------------------ CAMERA COMMANDS ------------------
        _server.on("/cam", HTTP_GET, [](AsyncWebServerRequest *request){

            if(request->hasParam("cmd")){
                String cmd = request->getParam("cmd")->value();

                if(cmd == "flip") {
                    sensor_t * s = esp_camera_sensor_get();
                    s->set_vflip(s, !s->status.vflip);
                }
                else if(cmd == "mirror") {
                    sensor_t * s = esp_camera_sensor_get();
                    s->set_hmirror(s, !s->status.hmirror);
                }
                else if(cmd == "wake") {
                    exitLowPowerMode();
                    lastValidRadarTime = millis();
                }
                else if(cmd == "reboot") {
                    ESP.restart();
                }
            }

            request->send(200,"text/plain","OK");
        });

        // ------------------ CONFIG POST ------------------
        _server.on("/config", HTTP_POST, [](AsyncWebServerRequest *request){
            if(request->hasParam("max_distance", true))
                cfg_max_dist = constrain(request->getParam("max_distance", true)->value().toInt(),1,100);
            if(request->hasParam("direction_mode", true))
                cfg_direction = constrain(request->getParam("direction_mode", true)->value().toInt(),0,2);
            if(request->hasParam("min_speed", true))
                cfg_min_speed = constrain(request->getParam("min_speed", true)->value().toInt(),0,120);
            if(request->hasParam("trigger_delay_ms", true))
                cfg_delay_time = constrain(request->getParam("trigger_delay_ms", true)->value().toInt(),0,30);
            if(request->hasParam("trigger_acc", true))
                cfg_trigger_acc = constrain(request->getParam("trigger_acc", true)->value().toInt(),1,10);
            if(request->hasParam("snr_limit", true))
                cfg_snr_limit = constrain(request->getParam("snr_limit", true)->value().toInt(),0,255);
            if(request->hasParam("rapid_threshold", true))
                cfg_rapid_threshold = constrain(request->getParam("rapid_threshold", true)->value().toInt(),5,150);
            if(request->hasParam("camera_timer_ms", true))
                cameraTimerMs = constrain(
                    request->getParam("camera_timer_ms", true)->value().toInt(),
                    3000,
                    60000
                );

            radarUpdatePending = true;

            request->send(200, "text/plain", "CONFIG UPDATED");
        });

        // ------------------ JSON DATA (legacy polling) ------------------
        _server.on("/data", HTTP_GET, [](AsyncWebServerRequest *request){

            char json[512];
            int offset = snprintf(json, sizeof(json),"{\"count\":%d,\"targets\":[",globalTargetCount);

            for(int i=0;i<globalTargetCount;i++){
                offset += snprintf(json+offset,sizeof(json)-offset,"{\"d\":%d,\"s\":%d,\"app\":%d}%s",activeTargets[i].distance,
                activeTargets[i].speed,activeTargets[i].approaching ? 1 : 0,(i < globalTargetCount-1) ? "," : "");
            }

            snprintf(json+offset,sizeof(json)-offset,"]}");
            request->send(200,"application/json",json);
        });

        _server.begin();
    }

    // --------------------------------------------------------
    // ONLY call when radar data changes
    // --------------------------------------------------------
    void sendRadarUpdate() {
        if(!_ws.count()) return;
        int offset = snprintf(
            radarJson,
            sizeof(radarJson),
            "{\"type\":\"radar\",\"timestamp\":%lu,\"count\":%d,\"targets\":[",
            millis(),
            globalTargetCount
        );
        for(int i = 0; i < globalTargetCount; i++) {
            // Prevent overflow
            if(offset >= sizeof(radarJson)) break;

            offset += snprintf(
                radarJson + offset,
                sizeof(radarJson) - offset,
                "{"
                "\"id\":%d,"
                "\"distance\":%d,"
                "\"speed\":%d,"
                "\"approaching\":%d,"
                "\"angle\":%d,"
                "\"snr\":%d"
                "\"smoothdis\":%d"
                "}%s",
                i,
                activeTargets[i].distance,
                activeTargets[i].speed,
                activeTargets[i].approaching ? 1 : 0,
                activeTargets[i].angle,      
                activeTargets[i].snr,  
                activeTargets[i].smoothedDist,    
                (i < globalTargetCount - 1) ? "," : ""
            );
        }

        if(offset < sizeof(radarJson)) {
            snprintf(radarJson + offset, sizeof(radarJson) - offset, "]}");
        }

        _ws.textAll(radarJson);
    }

    void cleanupWS() {
        _ws.cleanupClients();
    }

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

    <div class="btn-group">
    <button style="background:#00cfcf;color:black;font-weight:bold;"
            onclick="cmd('wake')">Wake Camera</button>
    </div>
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
    <label>Camera Sleep Timeout (ms)</label>
    <input type="number" id="camTimer" min="3000" max="60000" step="1000">
    </div>

    <button class="primary" onclick="save()">SAVE ALL CONFIGS</button>
    <button style="margin-top:10px;font-size:0.8em;" onclick="cmd('reboot')">Reboot Device</button>
    </div>

    </div>
    <div class="card">
    <div class="group-title">Live Radar (WebSocket)</div>
    <pre id="wslog" style="font-size:0.8em;height:120px;overflow:auto;background:#000;padding:10px;border-radius:6px;"></pre>
    </div>
    <script>
function g(id){ return document.getElementById(id).value; }
function s(id,val){ document.getElementById(id).value = val; }
// ---------------- WS Check ----------------
let ws;

function connectWS(){
    ws = new WebSocket(`ws://${location.host}/ws`);

    ws.onopen = () => {
        log("WS Connected");
    };

    ws.onmessage = (event) => {
        log("RX: " + event.data);
    };

    ws.onclose = () => {
        log("WS Disconnected - retrying...");
        setTimeout(connectWS, 2000);
    };

    ws.onerror = (err) => {
        log("WS Error");
    };
}

function log(msg){
    const el = document.getElementById("wslog");
    el.textContent += msg + "\n";
    el.scrollTop = el.scrollHeight;
}
// ---------------- CAMERA COMMAND ----------------
function cmd(type){
    fetch(`/cam?cmd=${type}`);
}

// ---------------- SAVE CONFIG ----------------
function save(){
    const params = new URLSearchParams({
        max_distance: g('dist'),
        direction_mode: g('dir'),
        min_speed: g('speed'),
        trigger_delay_ms: g('delay'),
        trigger_acc: g('acc'),
        snr_limit: g('snr'),
        rapid_threshold: g('rapid'),
        camera_timer_ms: g('camTimer')
    });

    fetch('/config', {
        method: 'POST',
        headers: {'Content-Type':'application/x-www-form-urlencoded'},
        body: params.toString()
    })
    .then(()=> {
        alert("Configuration Saved");
    });
}

// ---------------- LOAD CONFIG ON PAGE LOAD ----------------
window.onload = function(){
    
    connectWS();

    fetch('/config')
    .then(r => r.json())
    .then(cfg => {
        s('dist', cfg.dist);
        s('dir', cfg.dir);
        s('speed', cfg.speed);
        s('delay', cfg.delay);
        s('acc', cfg.acc);
        s('snr', cfg.snr);
        s('rapid', cfg.rapid);
        s('camTimer', cfg.camTimer);
    });
}

// ---------------- PROFILE PRESETS ----------------
const presets = {
    city:      {dist:40, speed:0, delay:0, acc:5, snr:6, rapid:15},
    highway:   {dist:100, speed:10, delay:0, acc:3, snr:4, rapid:20},
    rain:      {dist:60, speed:5, delay:1, acc:6, snr:10, rapid:15},
    commuter:  {dist:70, speed:5, delay:0, acc:4, snr:4, rapid:15},
    performance:{dist:100, speed:0, delay:0, acc:2, snr:4, rapid:25}
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
};

#endif