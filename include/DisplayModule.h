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

class DisplayModule {
private:
    Adafruit_ST7735 _display = Adafruit_ST7735(TFT_CS, TFT_DC, TFT_SDA, TFT_SCL, TFT_RST);
    
    // UI Boundary Constants
    const int roadTopY = 10;      
    const int roadBottomY = 130;  
    const int footerTopY = 134; 
    const int centerX = 64;

    void drawScale() {
        _display.setTextColor(0x528A); 
        _display.setTextSize(1);

        int max_d = (cfg_max_dist < 1) ? 1 : cfg_max_dist;
        int step = (max_d <= 10) ? 2 : 5;

        for (int m = step; m <= max_d; m += step) {
            int y = map(m, max_d, 0, roadTopY, roadBottomY);
            
            for(int x = 30; x < 100; x += 10) {
                _display.drawFastHLine(x, y, 4, 0x2104); 
            }
            
            _display.setCursor(2, y - 3);
            _display.print(m);
            _display.print("m");
        }
    }

    void drawRoad() {
        // Perspective Road Lines
        _display.drawLine(centerX - 15, roadTopY, centerX - 50, roadBottomY, 0x528A); 
        _display.drawLine(centerX + 15, roadTopY, centerX + 50, roadBottomY, 0x528A); 
    }

public:
    DisplayModule() {}

    void init() {
        _display.initR(INITR_BLACKTAB); 
        _display.setRotation(0); 
        _display.fillScreen(ST77XX_BLACK);
        drawStaticUI();
    }

    void drawStaticUI() {
        // Clear footer area specifically
        _display.fillRect(0, footerTopY, 128, 160 - footerTopY, 0x10A2); 
        _display.drawFastHLine(0, footerTopY, 128, ST7735_CYAN); 

        _display.setCursor(5, 145);
        _display.setTextColor(ST77XX_WHITE);
        _display.setTextSize(1);
        _display.print("SAFEBAIGE");
    }

    void updateMessage(String msg, uint16_t color = ST77XX_WHITE) {
        // Clear only the message area within the footer
        _display.fillRect(75, 145, 50, 12, 0x10A2); 
        _display.setCursor(75, 145);
        _display.setTextColor(color);
        _display.print(msg); 
    }

    void render(int count, RadarTarget *targets) {
        // 1. Wipe ONLY the road area to eliminate artifacts
        _display.fillRect(0, 0, 128, footerTopY, ST77XX_BLACK);
        
        // 2. Redraw structural elements
        drawRoad();
        drawScale();

        if (count <= 0 || targets == nullptr) return;

        // Ensure max_d isn't zero for math
        float max_d = (float)((cfg_max_dist < 1) ? 1 : cfg_max_dist);

        for (int i = 0; i < count; i++) {
            // Horizontal Logic
            int angleOffset = (int)targets[i].angle - 128; 
            int x_pos = map(angleOffset, -30, 30, 20, 108);

            // Vertical Logic
            float d = targets[i].smoothedDist;
            if (d > max_d) d = max_d;
            if (d < 0) d = 0;
            
            int y_pos = map(d, max_d, 0.0f, (float)roadTopY, (float)roadBottomY);

            // Perspective Scaling
            int carWidth = map(y_pos, roadTopY, roadBottomY, 6, 20);
            int carHeight = carWidth / 2;

            // Clamping to prevent footer/header artifacts
            if (y_pos + (carHeight / 2) >= footerTopY) y_pos = footerTopY - (carHeight / 2) - 1;
            if (y_pos - (carHeight / 2) <= roadTopY) y_pos = roadTopY + (carHeight / 2) + 1;

            uint16_t color = targets[i].approaching ? ST77XX_RED : ST77XX_GREEN;

            _display.fillRoundRect(x_pos - (carWidth/2), y_pos - (carHeight/2), carWidth, carHeight, 2, color);
            
            // Minimal speed label to reduce flicker
            if (y_pos > roadTopY + 15) {
                _display.setTextColor(ST77XX_WHITE);
                _display.setCursor(x_pos - 8, y_pos - (carHeight / 2) - 8);
                _display.print((int)targets[i].speed);
            }
        }
    }
};

#endif