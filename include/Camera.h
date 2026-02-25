#ifndef CAMERA
#define CAMERA
#include "esp_camera.h"

//CAMERA_MODEL_ESP32S3_EYE
#define PWDN_GPIO_NUM  -1
#define RESET_GPIO_NUM -1
#define XCLK_GPIO_NUM  15
#define SIOD_GPIO_NUM  4
#define SIOC_GPIO_NUM  5
#define Y2_GPIO_NUM 11
#define Y3_GPIO_NUM 9
#define Y4_GPIO_NUM 8
#define Y5_GPIO_NUM 10
#define Y6_GPIO_NUM 12
#define Y7_GPIO_NUM 18
#define Y8_GPIO_NUM 17
#define Y9_GPIO_NUM 16
#define VSYNC_GPIO_NUM 6
#define HREF_GPIO_NUM  7
#define PCLK_GPIO_NUM  13

class Camera {
private:
    camera_config_t camera_config = {
        .pin_pwdn = PWDN_GPIO_NUM,
        .pin_reset = RESET_GPIO_NUM,
        .pin_xclk = XCLK_GPIO_NUM,
        .pin_sccb_sda = SIOD_GPIO_NUM,
        .pin_sccb_scl = SIOC_GPIO_NUM,

        .pin_d7 = Y9_GPIO_NUM,
        .pin_d6 = Y8_GPIO_NUM,
        .pin_d5 = Y7_GPIO_NUM,
        .pin_d4 = Y6_GPIO_NUM,
        .pin_d3 = Y5_GPIO_NUM,
        .pin_d2 = Y4_GPIO_NUM,
        .pin_d1 = Y3_GPIO_NUM,
        .pin_d0 = Y2_GPIO_NUM,
        .pin_vsync = VSYNC_GPIO_NUM,
        .pin_href = HREF_GPIO_NUM,
        .pin_pclk = PCLK_GPIO_NUM,
        .xclk_freq_hz = 20000000,
        .ledc_timer = LEDC_TIMER_0,
        .ledc_channel = LEDC_CHANNEL_0,
        .pixel_format = PIXFORMAT_JPEG,
        .frame_size = FRAMESIZE_UXGA,   
        .jpeg_quality = 10, 
        .fb_count = 2,       
        .fb_location = CAMERA_FB_IN_PSRAM,
        .grab_mode = CAMERA_GRAB_LATEST,
    };
public:
    Camera(){}

    String init() {
        esp_err_t err = esp_camera_init(&camera_config);
        if (err != ESP_OK) {
            Serial.printf("Camera init failed: 0x%x", err);
            return "CAMERA: ERROR";
        }
        sensor_t *s = esp_camera_sensor_get();
        if (s->id.PID == OV3660_PID) {
            s->set_vflip(s, 1);        
            s->set_brightness(s, 1);   
            s->set_saturation(s, -2); 
        }
        
        if (camera_config.pixel_format == PIXFORMAT_JPEG) {
            s->set_framesize(s, FRAMESIZE_VGA);
        }

        // Reduce Motion Blur (Crucial for moving cars)
        s->set_quality(s, 10);     
        s->set_gain_ctrl(s, 1);     // Auto gain on
        s->set_exposure_ctrl(s, 1); // Auto exposure on
        s->set_hmirror(s, 0);
        
        return "OK";
    }
};

#endif