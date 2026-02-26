# SafeBaige Ultra

Hardware implementation of the original SafeBaige project.
This version runs on a real ESP32-S3 (16n8r), a HLK-LD2451 radar, a camera module (ov36600), a headless mode for phone-driven UI or one with a display (ST7735).

The system keeps the Garmin Varia-inspired radar tracking concept, while extending it with structured networking and logic for a future client application designed to handle visualization and YOLO.

## System Architecture

- **Radar Core**: Interfaces with a real HLK-LD2451 mmWave radar module over UART (115200 baud)
- **Embedded Controller**: Runs on a ESP32-S3 using Arduino framework under PlatformIO.
- **Headless Design**: Display support is optional and compile-time controlled. In headless mode, the ESP focuses purely on sensing and data streaming.
- **Camera**: ESP32 camera for onboard capture and streaming for external validation logic.
- **Realtime Tracking**: WebSocket server streams radar frames to a connected client.
- **IoT Connectivity**: 
    -*REST endpoints*: Provides endpoints to configure the radar and camera hardware and other global variables with an added simple web ui.
    -*AP wifi mode*: Creates a wifi AP for connection (SSID: "SafeBaige" | PW: "drivesafe") | can be changed in NetworkManager.

## Project Structure

| File             | Responsibility                                                     |
| ---------------- | ------------------------------------------------------------------ |
| Camera.h         | Camera class for configuration setup uses ("esp_camera.h")         |
| DisplayModule.h  | Optional SPI display renderer (compiled out in headless mode).     |
| FilterModule.h   | Smooths radar jitter and reduces false movement artifacts.         |
| LD2451_Defines.h | HLK-LD2451 data structure                                          |
| NetworkManager.h | Manages WiFi, WebSocket server, heartbeat, and JSON serialization. |
| RadarConfig.h    | Handles Radar parameters configuration                             |
| RadarParser.h    | Decodes HLK-LD2451 binary UART protocol frames.                    |
| StreamServer.cpp | Simplified camera stream server logic (from the arduino examples)  |
| main.cpp         | System initialization, radar polling loop, network pump.           |

## Build configuration

The project supports a headless configuration via compile-time flags on platformio.ini

```
build_flags =
    -DUSE_DISPLAY=0
```

## Hardware Mapping 

| Component  | ESP32-S3 Pin    | Protocol    |
| ---------- | --------------- | ----------- |
| Radar TX   | 1  | UART        |
| Radar RX   | 2 (UART RX)  | UART        |
| Camera     | ESP32-S3 native | DVP         |
| Screen     | 38 to 42 | SPI         |

Those can all be changed in the respective files.

## IoT API Endpoints

Uses ESPmDNS for setting up a multicast DNS (Safebaige.local) | can also connect using the esp ip (usually defaults to 192.168.4.1 / this can also be "found" since it's the given network gateway)

### HTTP Endpoints

#### GET /

Serves the built-in web interface (HTML control panel). With some radar settings profiles (untested in real life scenarios, just an estimation to what to use)

#### GET /config

Returns the current radar and system configuration.

| Field      | Description                                    |
| ---------- | ---------------------------------------------- |
| `dist`     | Max detection distance (meters)                |
| `dir`      | Direction mode (0=away, 1=approaching, 2=both) |
| `speed`    | Minimum speed threshold (km/h)                 |
| `delay`    | Trigger delay time (seconds)                   |
| `acc`      | Trigger accuracy (1–10)                        |
| `snr`      | Radar SNR limit                                |
| `rapid`    | Speed threshold for RED alert                  |
| `camTimer` | Camera sleep timeout (ms)                      |


#### POST /config

Updates radar and system configuration.

| Parameter          | Range      | Description                   |
| ------------------ | ---------- | ----------------------------- |
| `max_distance`     | 1–100      | Max radar distance (meters)   |
| `direction_mode`   | 0–2        | 0=away, 1=approaching, 2=both |
| `min_speed`        | 0–120      | Minimum speed (km/h)          |
| `trigger_delay_ms` | 0–30       | Delay in seconds              |
| `trigger_acc`      | 1–10       | Accuracy level                |
| `snr_limit`        | 0–255      | Radar SNR threshold           |
| `rapid_threshold`  | 5–150      | Speed threshold for red alert |
| `camera_timer_ms`  | 3000–60000 | Camera sleep timeout (ms)     |

#### GET /cam

Sends camera and/or system control commands.

| Command  | Description                     |
| -------- | ------------------------------- |
| `flip`   | Toggle vertical flip            |
| `mirror` | Toggle horizontal mirror        |
| `wake`   | Wake camera from low power mode |
| `reboot` | Restart device                  |

#### GET /data

Legacy polling endpoint for radar target data (non-WebSocket fallback).

| Field   | Description                        |
| ------- | ---------------------------------- |
| `count` | Number of detected targets         |
| `d`     | Distance (meters)                  |
| `s`     | Speed (km/h)                       |
| `app`   | Approaching flag (1=true, 0=false) |


### WebSocket API

#### /ws

Provides real-time radar updates.

The server:

- Sends radar updates when target data changes
- Sends heartbeat pings every 5 seconds
- Automatically cleans up disconnected clients

Example message:
```
{
  "type": "radar",
  "timestamp": 12345678,
  "count": 1,
  "targets": [
    {
      "id": 0,
      "distance": 42,
      "speed": 38,
      "approaching": 1,
      "angle": -12,
      "snr": 8,
      "smoothdis": 40
    }
  ]
}
```
Field Descriptions

| Field         | Description                |
| ------------- | -------------------------- |
| `type`        | Message type (`radar`)     |
| `timestamp`   | System uptime in ms        |
| `count`       | Number of detected targets |
| `id`          | Target index               |
| `distance`    | Raw distance (meters)      |
| `speed`       | Speed (km/h)               |
| `approaching` | 1=approaching, 0=away      |
| `angle`       | Target angle               |
| `snr`         | Signal-to-noise ratio      |
| `smoothdis`   | Smoothed distance value    |

### Camera Stream

Served at:

```
http://safebaige.local:81/stream
```

