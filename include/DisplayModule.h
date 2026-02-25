#ifndef DISPLAY_MODULE_H
#define DISPLAY_MODULE_H

#include <Adafruit_GFX.h>
#include <Adafruit_ST7735.h>
#include "LD2451_Defines.h"

#define TFT_SCL    38
#define TFT_SDA    39
#define TFT_DC     40
#define TFT_RST    41
#define TFT_CS     42

extern uint8_t cfg_max_dist;
extern uint8_t cfg_rapid_threshold;

class DisplayModule {
private:
    Adafruit_ST7735 _display = Adafruit_ST7735(TFT_CS, TFT_DC, TFT_SDA, TFT_SCL, TFT_RST);
    
    // UI Boundary Constants
    const int roadTopY = 10;      // Far horizon
    const int roadBottomY = 130;  // Near edge
    const int footerTopY = 134; 
    const int centerX = 64;

    void drawScale() {
        // Draw distance markers based on current cfg_max_dist
        _display.setTextColor(0x528A); // Dark Gray
        _display.setTextSize(1);

        // Draw scale every 2m or 5m depending on range
        int step = (cfg_max_dist <= 10) ? 2 : 5;

        for (int m = step; m <= cfg_max_dist; m += step) {
            // Map meters to Y position (cfg_max_dist is far, 0 is near)
            int y = map(m, cfg_max_dist, 0, roadTopY, roadBottomY);
            
            // Draw a faint dashed line for the distance marker
            for(int x = 30; x < 100; x += 10) {
                _display.drawFastHLine(x, y, 4, 0x2104); 
            }
            
            _display.setCursor(5, y - 3);
            _display.print(m);
            _display.print("m");
        }
    }

    void drawRoad() {
        // Perspective Road Lines
        // Far point (narrow) to Near point (wide)
        _display.drawLine(centerX - 15, roadTopY, centerX - 50, roadBottomY, 0x528A); 
        _display.drawLine(centerX + 15, roadTopY, centerX + 50, roadBottomY, 0x528A); 
    }

public:
    DisplayModule() {}

    void init() {
        pinMode(TFT_RST, OUTPUT);
        digitalWrite(TFT_RST, HIGH); delay(10);
        digitalWrite(TFT_RST, LOW);  delay(10);
        digitalWrite(TFT_RST, HIGH); delay(10);

        _display.initR(INITR_BLACKTAB); 
        _display.setRotation(0); // Vertical orientation
        drawStaticUI();
    }

    void drawStaticUI() {
        _display.fillScreen(ST77XX_BLACK);
        
        // Footer Area
        _display.fillRect(0, footerTopY, 128, 160 - footerTopY, 0x10A2); 
        _display.drawFastHLine(0, footerTopY, 128, ST7735_CYAN); 

        _display.setCursor(5, 145);
        _display.setTextColor(ST77XX_WHITE);
        _display.setTextSize(1);
        _display.print("SAFEBAIGE");
    }

    void updateMessage(String msg, uint16_t color = ST77XX_WHITE) {
        _display.fillRect(80, 145, 45, 12, 0x10A2); 
        _display.setCursor(80, 145);
        _display.setTextSize(1);
        _display.setTextColor(color);
        _display.print(msg); 
    }

    void render(int count, RadarTarget *targets) {
        _display.fillRect(0, 0, 128, footerTopY, ST77XX_BLACK);
        
        drawRoad();
        drawScale();

        if (count <= 0 || targets == nullptr) return;

        for (int i = 0; i < count; i++) {
            // Horizontal Position (Angle)
            int angleOffset = (int)targets[i].angle - 128; 
            int x_pos = map(angleOffset, -30, 30, 20, 108);

            // Vertical Position (Distance)
            float d = targets[i].smoothedDist;
            if (d > cfg_max_dist) d = cfg_max_dist;
            
            int y_pos = map((float)d, (float)cfg_max_dist, 0.0f, (float)roadTopY, (float)roadBottomY);

            // Perspective Scaling
            int carWidth = map(y_pos, roadTopY, roadBottomY, 6, 22);
            int carHeight = carWidth / 1.5;

            if (y_pos + (carHeight / 2) >= footerTopY) {
                y_pos = footerTopY - (carHeight / 2) - 1; 
            }
            

            if (y_pos - (carHeight / 2) < roadTopY) {
                y_pos = roadTopY + (carHeight / 2);
            }

            uint16_t color = targets[i].approaching ? ST77XX_RED : ST77XX_GREEN;
            if (targets[i].approaching && targets[i].speed > cfg_rapid_threshold) {
                color = ST77XX_RED; // Rapid Approach alert
            }    
            // Draw the car body
            _display.fillRoundRect(x_pos - (carWidth/2), y_pos - (carHeight/2), carWidth, carHeight, 3, color);
            
            // Draw detail (only if car is big enough to see it)
            if (carWidth > 10) {
                uint16_t detailCol = targets[i].approaching ? ST77XX_WHITE : 0x7BEF;
                _display.fillRect(x_pos - (carWidth/4), y_pos - (carHeight/4), carWidth/2, 2, detailCol);
            }

            // Label the Speed (Constrain text so it doesn't bleed into footer either)
            if (y_pos > roadTopY + 10) {
                _display.setTextColor(ST77XX_WHITE);
                _display.setCursor(x_pos + (carWidth/2) + 2, y_pos - 4);
                _display.print(targets[i].speed);
                _display.print("k");
            }
        }
    }
};

#endif