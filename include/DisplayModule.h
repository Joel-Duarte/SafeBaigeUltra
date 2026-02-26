#ifndef DISPLAY_MODULE_H
#define DISPLAY_MODULE_H

#include <Adafruit_GFX.h>
#include <Adafruit_ST7735.h>
#include <math.h>
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

    const int roadTopY = 10;
    const int roadBottomY = 130;
    const int footerTopY = 134;
    const int centerX = 64;

    RadarTarget previousTargets[5];
    int previousCount = 0;

    struct CarRect {
        int x;
        int y;
        int w;
        int h;
    };

    CarRect previousRects[5];

    // -------------------------
    // Smooth Perspective Mapping (vertical only)
    // -------------------------
    int distanceToY(float d) {

        float visualMax = (float)cfg_max_dist;

        if (d > visualMax)
            d = visualMax;

        float normalized = d / visualMax;

        // Smooth curve for near emphasis
        float curved = 1.0f - pow(1.0f - normalized, 2.2f);

        return roadTopY + (roadBottomY - roadTopY) * (1.0f - curved);
    }

    // -------------------------
    // Smart Scale Drawing
    // -------------------------
    void drawScale() {

        _display.setTextColor(0x528A);
        _display.setTextSize(1);

        int visualMax = cfg_max_dist;

        // 5m increments to 20
        for (int m = 5; m <= 20; m += 5) {

            if (m > visualMax) break;

            int y = distanceToY(m);

            for (int x = 30; x < 100; x += 12)
                _display.drawFastHLine(x, y, 4, 0x2104);

            _display.setCursor(5, y - 3);
            _display.print(m);
            _display.print("m");
        }

        // 10m increments to 50
        for (int m = 30; m <= 50; m += 10) {

            if (m > visualMax) break;

            int y = distanceToY(m);

            for (int x = 30; x < 100; x += 12)
                _display.drawFastHLine(x, y, 4, 0x2104);

            _display.setCursor(5, y - 3);
            _display.print(m);
            _display.print("m");
        }

        // Only draw max above 50
        if (visualMax > 50) {

            int y = distanceToY(visualMax);

            for (int x = 30; x < 100; x += 12)
                _display.drawFastHLine(x, y, 4, 0x2104);

            _display.setCursor(5, y - 3);
            _display.print(visualMax);
            _display.print("m");
        }
    }

    void drawRoad() {

        int horizonWidth = 12;
        int bottomWidth = 50;

        _display.drawLine(centerX - horizonWidth, roadTopY,centerX - bottomWidth, roadBottomY, 0x528A);

        _display.drawLine(centerX + horizonWidth, roadTopY,centerX + bottomWidth, roadBottomY, 0x528A);
    }

    void drawBackground() {
        _display.fillRect(0, 0, 128, footerTopY, ST77XX_BLACK);
        drawRoad();
        drawScale();
    }

    // -------------------------
    // Clean erase using stored rectangles
    // -------------------------
    void erasePreviousCars() {

        for (int i = 0; i < previousCount; i++) {

            CarRect r = previousRects[i];

            _display.fillRect(
                r.x - 2,
                r.y - 2,
                r.w + 4,
                r.h + 4,
                ST77XX_BLACK
            );
        }
        
        previousCount = 0;
    }

public:
    DisplayModule() {}

    void init() {

        pinMode(TFT_RST, OUTPUT);
        digitalWrite(TFT_RST, HIGH); delay(10);
        digitalWrite(TFT_RST, LOW);  delay(10);
        digitalWrite(TFT_RST, HIGH); delay(10);

        _display.initR(INITR_BLACKTAB);
        _display.setRotation(0);
        _display.fillScreen(ST77XX_BLACK);

        // Footer
        _display.fillRect(0, footerTopY, 128, 160 - footerTopY, 0x10A2);
        _display.drawFastHLine(0, footerTopY, 128, ST7735_CYAN);

        _display.setCursor(5, 145);
        _display.setTextColor(ST77XX_WHITE);
        _display.setTextSize(1);
        _display.print("SAFEBAIGE");

        drawBackground();
    }

    void redrawBackground() {
        drawBackground();
    }

    void updateMessage(String msg, uint16_t color = ST77XX_WHITE) {

        _display.fillRect(80, 145, 45, 12, 0x10A2);

        _display.setCursor(80, 145);
        _display.setTextSize(1);
        _display.setTextColor(color);
        _display.print(msg);
    }

    void render(int count, RadarTarget *targets) {

        erasePreviousCars();

        if (count <= 0 || targets == nullptr)
            return;

        for (int i = 0; i < count; i++) {

            float d = targets[i].smoothedDist;
            int y = distanceToY(d);

            int w = map(y, roadTopY, roadBottomY, 6, 22);
            int h = w / 2;

            if (y + (h/2) >= footerTopY)
                y = footerTopY - (h/2) - 1;

            int angleOffset = targets[i].angle;
            int x = map(angleOffset, -30, 30, 20, 108);

            uint16_t color =
                (targets[i].approaching && targets[i].speed > cfg_rapid_threshold)
                ? ST77XX_RED
                : (targets[i].approaching ? ST77XX_ORANGE : ST77XX_GREEN);

            int drawX = x - (w/2);
            int drawY = y - (h/2);

            _display.fillRoundRect(
                drawX,
                drawY,
                w,
                h,
                3,
                color
            );

            // Store exact rectangle for next erase
            previousRects[i] = { drawX, drawY, w, h };
            previousTargets[i] = targets[i];
        }

        previousCount = count;
    }
};

#endif