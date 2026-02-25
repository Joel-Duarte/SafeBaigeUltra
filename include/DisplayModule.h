#ifndef DISPLAY_MODULE_H
#define DISPLAY_MODULE_H

#include <Adafruit_GFX.h>
#include <Adafruit_ST7735.h>
//#include "RadarConfig.h"

#define TFT_SCL    38   // Clock (SCL)
#define TFT_SDA    39   // Data (SDA)
#define TFT_DC     40   // Data/Command (A0)
#define TFT_RST    41  // Reset
#define TFT_CS     42  // Chip Select

class DisplayModule {
private:
    Adafruit_ST7735 _display = Adafruit_ST7735(TFT_CS, TFT_DC, TFT_SDA, TFT_SCL, TFT_RST);

public:
    DisplayModule() {}

    void init() {
        // Manually toggle Reset pin to wake up the controller
        pinMode(TFT_RST, OUTPUT);
        digitalWrite(TFT_RST, HIGH);
        delay(50);
        digitalWrite(TFT_RST, LOW);
        delay(50);
        digitalWrite(TFT_RST, HIGH);
        delay(50);

        // Try BLACKTAB first; if colors are weird, swap to GREENTAB
        _display.initR(INITR_BLACKTAB); 
        
        _display.setRotation(0); // Landscape
        _display.fillScreen(ST77XX_BLACK);

        _display.setCursor(10, 10);
        _display.setTextColor(ST7735_CYAN);
        _display.setTextSize(2);
        _display.println("SAFEBAIGE");
        //Serial.println("Display Initialized");
    }

    void updateMessage(String msg, uint16_t color = ST77XX_WHITE) {
        _display.fillRect(0, 50, 160, 30, ST77XX_BLACK); 
        
        _display.setCursor(10, 60);
        _display.setTextSize(1);
        _display.setTextColor(color);
        _display.print(msg);
    }

    void drawStatusBar(bool phoneConnected, bool isRecording) {
        _display.setTextSize(1);
        _display.setCursor(5, 5);
        
        if (phoneConnected) {
            _display.setTextColor(ST77XX_GREEN);
            _display.print("PHONE:OK");
        } else {
            _display.setTextColor(ST77XX_RED);
            _display.print("PHONE:--");
        }

        if (isRecording) {
            if ((millis() / 500) % 2 == 0) {
                _display.fillCircle(140, 8, 3, ST77XX_RED);
                _display.setCursor(146, 5);
                _display.setTextColor(ST77XX_WHITE);
                _display.print("REC");
            } else {
                _display.fillRect(135, 5, 25, 10, ST77XX_BLACK);
            }
        }
    }

    //void render(int count, RadarTarget *targets, bool phoneConnected, bool isRecording) {
    //    _display.fillScreen(ST77XX_BLACK);
    //    drawStatusBar(phoneConnected, isRecording);

    //    int roadX = 130; 
    //    _display.drawLine(roadX, 128, roadX, 20, ST77XX_WHITE); 

    //    for (int i = 0; i < count; i++) {
    //        int yPos = map(targets[i].distance, 0, 100, 25, 120);
    //        int xOffset = map(targets[i].angle, 0, 255, -60, 60);
    //        
    //        if (currentTrafficSide == LEFT_HAND_DRIVE) {
    //            xOffset = -xOffset;
    //        }

    //        uint16_t color = (targets[i].distance < 15) ? ST77XX_RED : ST77XX_ORANGE;
    //        _display.fillCircle(roadX + xOffset, yPos, 4, color); 
    //        
    //        int barWidth = map(targets[i].distance, 0, 100, 60, 0);
    //        _display.fillRect(5, 25 + (i * 15), barWidth, 10, ST77XX_CYAN);
    //        
    //        _display.setCursor(70, 25 + (i * 15));
    //        _display.setTextColor(ST77XX_WHITE);
    //        _display.print(targets[i].distance); _display.print("m");
    //    }
    //}
    
    void showClear(bool phoneConnected) {
        _display.fillScreen(ST77XX_BLACK);
        drawStatusBar(phoneConnected, false);
        _display.setCursor(45, 60);
        _display.setTextColor(ST77XX_GREEN);
        _display.print("ROAD CLEAR");
    }
};

#endif