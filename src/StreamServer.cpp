#include "StreamServer.h"
#include "esp_http_server.h"
#include "esp_camera.h"
#include "DisplayModule.h"
#include <Arduino.h>
#include <Adafruit_ST7735.h>

extern DisplayModule ui;
// Standard MJPEG headers
#define PART_BOUNDARY "123456789000000000000987654321"
static const char* _STREAM_CONTENT_TYPE = "multipart/x-mixed-replace;boundary=" PART_BOUNDARY;
static const char* _STREAM_BOUNDARY = "\r\n--" PART_BOUNDARY "\r\n";
static const char* _STREAM_PART = "Content-Type: image/jpeg\r\nContent-Length: %u\r\n\r\n";
String streamStatus = "STREAM: READY";

static esp_err_t stream_handler(httpd_req_t *req) {
    camera_fb_t * fb = NULL;
    esp_err_t res = ESP_OK;

    res = httpd_resp_set_type(req, _STREAM_CONTENT_TYPE);
    if(res != ESP_OK) return res;
    ui.updateMessage("STREAM: ACTIVE", ST77XX_CYAN);
    while(true) {
        fb = esp_camera_fb_get();
        if (!fb) {
            Serial.println("Camera capture failed");
            ui.updateMessage("STREAM: ERROR", ST77XX_RED);
            res = ESP_FAIL;
        } else {
            // Send headers
            char part_buf[64];
            size_t hlen = snprintf((char *)part_buf, 64, _STREAM_PART, fb->len);
            res = httpd_resp_send_chunk(req, (const char *)part_buf, hlen);
            
            // Send frame
            if(res == ESP_OK) res = httpd_resp_send_chunk(req, (const char *)fb->buf, fb->len);
            
            // Send boundary
            if(res == ESP_OK) res = httpd_resp_send_chunk(req, _STREAM_BOUNDARY, strlen(_STREAM_BOUNDARY));
            
            esp_camera_fb_return(fb); // CRITICAL: return the buffer
        }

        if(res != ESP_OK) {
            ui.updateMessage("STREAM: CLOSED", ST77XX_ORANGE);
            break;
        }
        // Allow other background tasks (like WiFi) to run
        vTaskDelay(1 / portTICK_PERIOD_MS); 
    }
    return res;
}

void startCameraServer() {
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.server_port = 81;
    config.ctrl_port = 32769;
    // For S3 N16R8, we can increase the stack size if the stream is heavy
    config.stack_size = 8192; 

    httpd_uri_t stream_uri = {
        .uri       = "/stream",
        .method    = HTTP_GET,
        .handler   = stream_handler,
        .user_ctx  = NULL
    };

    httpd_handle_t stream_httpd = NULL;
    if (httpd_start(&stream_httpd, &config) == ESP_OK) {
        httpd_register_uri_handler(stream_httpd, &stream_uri);
        Serial.println("Stream Server started on port 81 at /stream");
    }
}