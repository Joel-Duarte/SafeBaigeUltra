#include "StreamServer.h"
#include "esp_http_server.h"
#include "esp_camera.h"
#include <Arduino.h>

// ===== MJPEG headers =====
#define PART_BOUNDARY "123456789000000000000987654321"
static const char* _STREAM_CONTENT_TYPE = "multipart/x-mixed-replace;boundary=" PART_BOUNDARY;
static const char* _STREAM_BOUNDARY = "\r\n--" PART_BOUNDARY "\r\n";
static const char* _STREAM_PART = "Content-Type: image/jpeg\r\nContent-Length: %u\r\n\r\n";

static bool streamLowPower = false;

void enterLowPowerMode() {
    streamLowPower = true;
}

void exitLowPowerMode() {
    streamLowPower = false;
}

bool isLowPower() {
    return streamLowPower;
}

// ===== 1x1 black JPEG =====
static const uint8_t black_jpeg[] = {
  0xFF,0xD8,0xFF,0xDB,0x00,0x43,0x00,
  0x08,0x06,0x06,0x07,0x06,0x05,0x08,0x07,0x07,0x07,
  0x09,0x09,0x08,0x0A,0x0C,0x14,0x0D,0x0C,0x0B,0x0B,
  0x0C,0x19,0x12,0x13,0x0F,0x14,0x1D,0x1A,0x1F,0x1E,
  0x1D,0x1A,0x1C,0x1C,0x20,0x24,0x2E,0x27,0x20,0x22,
  0x2C,0x23,0x1C,0x1C,0x28,0x37,0x29,0x2C,0x30,0x31,
  0x34,0x34,0x34,0x1F,0x27,0x39,0x3D,0x38,0x32,0x3C,
  0x2E,0x33,0x34,0x32,
  0xFF,0xD9
};

static esp_err_t stream_handler(httpd_req_t *req)
{
    esp_err_t res = ESP_OK;

    res = httpd_resp_set_type(req, _STREAM_CONTENT_TYPE);
    if (res != ESP_OK) return res;

    unsigned long lastLowPowerFrame = 0;

    while (true)
    {
        if (!streamLowPower)
        {
            // ===== NORMAL MODE =====
            camera_fb_t *fb = esp_camera_fb_get();

            if (!fb)
            {
                res = ESP_FAIL;
            }
            else
            {
                char part_buf[64];
                size_t hlen = snprintf(part_buf, sizeof(part_buf),_STREAM_PART, fb->len);

                res = httpd_resp_send_chunk(req, part_buf, hlen);
                if (res == ESP_OK)
                    res = httpd_resp_send_chunk(req,(const char*)fb->buf,fb->len);
                if (res == ESP_OK)
                    res = httpd_resp_send_chunk(req,_STREAM_BOUNDARY,strlen(_STREAM_BOUNDARY));

                esp_camera_fb_return(fb);
            }

            vTaskDelay(1);
        }
        else
        {
            // ===== LOW POWER MODE =====
            unsigned long now = millis();

            if (now - lastLowPowerFrame >= 2000)
            {
                char part_buf[64];
                size_t hlen = snprintf(part_buf, sizeof(part_buf),_STREAM_PART,sizeof(black_jpeg));

                res = httpd_resp_send_chunk(req, part_buf, hlen);
                if (res == ESP_OK)
                    res = httpd_resp_send_chunk(req,(const char*)black_jpeg,sizeof(black_jpeg));
                if (res == ESP_OK)
                    res = httpd_resp_send_chunk(req,_STREAM_BOUNDARY,strlen(_STREAM_BOUNDARY));

                lastLowPowerFrame = now;
            }

            vTaskDelay(50); 
        }

        if (res != ESP_OK)
            break;
    }

    return res;
}

void startCameraServer()
{
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.server_port = 81;
    config.ctrl_port = 32769;
    config.stack_size = 8192;

    httpd_uri_t stream_uri = {
        .uri       = "/stream",
        .method    = HTTP_GET,
        .handler   = stream_handler,
        .user_ctx  = NULL
    };

    httpd_handle_t stream_httpd = NULL;

    if (httpd_start(&stream_httpd, &config) == ESP_OK)
    {
        httpd_register_uri_handler(stream_httpd, &stream_uri);
    }
}