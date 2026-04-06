#ifndef LED_MANAGER_H
#define LED_MANAGER_H

#include <FastLED.h>

class LedManager {
public:
    LedManager(int numLeds) : _numLeds(numLeds) {
        _leds = new CRGB[_numLeds];
    }

    // Initialize FastLED on Pin 41
    void begin() {
        FastLED.addLeds<WS2812B, 41, GRB>(_leds, _numLeds);
        FastLED.setBrightness(100); 
        FastLED.clear();
        FastLED.show();
    }

    // Blinks green twice on successful boot
    void playBootSequence() {
        for (int i = 0; i < 2; i++) {
            fill_solid(_leds, _numLeds, CRGB::Green);
            FastLED.show();
            delay(400);
            fill_solid(_leds, _numLeds, CRGB::Black);
            FastLED.show();
            delay(400);
        }
        // Transitions to solid red as the "ready" state
        setBaseState();
    }

    void playErrorSequence() {
        for (int i = 0; i < 2; i++) {
            fill_solid(_leds, _numLeds, CRGB::Red);
            FastLED.show();
            delay(400);
            fill_solid(_leds, _numLeds, CRGB::White);
            FastLED.show();
            delay(400);
        }
        // Transitions to solid red as the "ready" state
        setBaseState();
    }

    // Set the strip to solid red
    void setBaseState() {
        fill_solid(_leds, _numLeds, CRGB::Red);
        FastLED.show();
    }

    // Logic for braking/deceleration blink
    void updateBraking(bool isBraking) {
        if (isBraking) {
            // Uses FastLED's internal timer to blink without blocking the MPU readings
            EVERY_N_MILLISECONDS(150) {
                static bool toggle = false;
                if (toggle) {
                    fill_solid(_leds, _numLeds, CRGB::Red);
                } else {
                    fill_solid(_leds, _numLeds, CRGB::Black);
                }
                FastLED.show();
                toggle = !toggle;
            }
        } else {
            // Ensure we return to solid red when not braking
            static bool wasBraking = false;
            if (wasBraking) {
                setBaseState();
                wasBraking = false;
            }
        }
    }

private:
    int _numLeds;
    CRGB* _leds;
};

#endif