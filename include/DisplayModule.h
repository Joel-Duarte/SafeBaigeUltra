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
    const int roadTopY = 2;      // Top of screen
    const int footerTopY = 134; 
    const int roadBottomY = 130; // Maximum Y for a car 
    const int centerX = 64;

public:
    DisplayModule() {}

    void init() {
        pinMode(TFT_RST, OUTPUT);
        digitalWrite(TFT_RST, HIGH); delay(10);
        digitalWrite(TFT_RST, LOW);  delay(10);
        digitalWrite(TFT_RST, HIGH); delay(10);

        _display.initR(INITR_BLACKTAB); 
        _display.setRotation(0); 
        drawStaticUI();
    }

    void drawStaticUI() {
        _display.fillScreen(ST77XX_BLACK);
        
        // Static Footer (Y: 134 to 160)
        _display.fillRect(0, footerTopY, 128, 160 - footerTopY, 0x10A2); 
        _display.drawFastHLine(0, footerTopY, 128, ST7735_CYAN); 

        _display.setCursor(5, 145);
        _display.setTextColor(ST77XX_WHITE);
        _display.setTextSize(1);
        _display.print("SAFEBAIGE");

    }

    void updateMessage(String msg, uint16_t color = ST77XX_WHITE) {
        _display.fillRect(80, 145, 30, 15, 0x10A2); 
        _display.setCursor(80, 145);
        _display.setTextSize(1);
        _display.setTextColor(color);
        _display.print(msg.substring(0, 3)); 
    }

    void render(int count, RadarTarget *targets) {
        _display.fillRect(0, 0, 128, footerTopY, ST77XX_BLACK);
        
        _display.drawLine(centerX - 35, roadBottomY, centerX - 10, roadTopY, 0x528A); 
        _display.drawLine(centerX + 35, roadBottomY, centerX + 10, roadTopY, 0x528A); 

        if (count <= 0 || targets == nullptr) return;

        for (int i = 0; i < count; i++) {
            int angleOffset = (int)targets[i].angle - 128; 
            int xOffset = map(angleOffset, -25, 25, -45, 45);
            int targetX = constrain(centerX + xOffset, 15, 113);

            int currentDist = (int)targets[i].distance;
            int yPos = map((float)targets[i].smoothedDist, (float)cfg_max_dist, 0.0f, (float)roadTopY, (float)roadBottomY);

            int carSize = map(yPos, roadTopY, roadBottomY, 4, 16);
            if (yPos + (carSize/1.5) > footerTopY - 2) {
                yPos = footerTopY - (carSize/1.5) - 2;
            }

            uint16_t color = targets[i].approaching ? ST77XX_RED : ST77XX_GREEN;

            // Draw Car
            _display.fillRoundRect(targetX - (carSize/2), yPos, carSize, carSize/1.5, 2, color);
            
            // Label Distance
            _display.setTextColor(ST77XX_WHITE);
            _display.setCursor(targetX + (carSize/2) + 2, yPos);
            _display.print(currentDist);
            _display.print("m");
        }
    }
};

#endif