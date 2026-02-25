#include "StreamServer.h"
#include "esp_http_server.h"
#include "esp_camera.h"
#include "DisplayModule.h"
#include <Arduino.h>

extern DisplayModule ui;

// Standard MJPEG headers
#define PART_BOUNDARY "123456789000000000000987654321"
static const char* _STREAM_CONTENT_TYPE = "multipart/x-mixed-replace;boundary=" PART_BOUNDARY;
static const char* _STREAM_BOUNDARY = "\r\n--" PART_BOUNDARY "\r\n";
static const char* _STREAM_PART = "Content-Type: image/jpeg\r\nContent-Length: %u\r\n\r\n";

// Global handle to allow stopping the server
static httpd_handle_t stream_httpd = NULL;

static esp_err_t stream_handler(httpd_req_t *req) {
    camera_fb_t * fb = NULL;
    esp_err_t res = ESP_OK;
    size_t _jpg_buf_len = 0;
    uint8_t * _jpg_buf = NULL;

    res = httpd_resp_set_type(req, _STREAM_CONTENT_TYPE);
    if(res != ESP_OK) return res;
    ui.updateMessage("STREAMING", ST77XX_CYAN);

    while(true) {
        fb = esp_camera_fb_get();
        if (!fb) {
            res = ESP_FAIL;
        } else {
            // Frame is already JPEG based on your Camera.h config
            char part_buf[64];
            size_t hlen = snprintf((char *)part_buf, 64, _STREAM_PART, fb->len);
            
            res = httpd_resp_send_chunk(req, (const char *)part_buf, hlen);
            if(res == ESP_OK) res = httpd_resp_send_chunk(req, (const char *)fb->buf, fb->len);
            if(res == ESP_OK) res = httpd_resp_send_chunk(req, _STREAM_BOUNDARY, strlen(_STREAM_BOUNDARY));
            
            esp_camera_fb_return(fb); 
        }

        if(res != ESP_OK) {
            break;
        }
        
        // Yield to other FreeRTOS tasks
        vTaskDelay(10 / portTICK_PERIOD_MS); 
    }

    ui.updateMessage("CMR READY", ST77XX_GREEN);
    return res;
}

void startCameraServer() {
    // Prevent starting multiple instances
    if (stream_httpd != NULL) return;

    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.server_port = 81;
    config.ctrl_port = 32769;
    config.stack_size = 8192; 
    config.task_priority = 5; // Moderate priority to balance with Radar

    httpd_uri_t stream_uri = {
        .uri       = "/stream",
        .method    = HTTP_GET,
        .handler   = stream_handler,
        .user_ctx  = NULL
    };

    if (httpd_start(&stream_httpd, &config) == ESP_OK) {
        httpd_register_uri_handler(stream_httpd, &stream_uri);
        ui.updateMessage("STREAM: ON", ST77XX_GREEN);
    }
}

void stopCameraServer() {
    if (stream_httpd != NULL) {
        httpd_stop(stream_httpd);
        stream_httpd = NULL;
        Serial.println("Stream Server stopped");
        ui.updateMessage("STREAM: OFF", ST77XX_ORANGE);
    }
}