#ifndef MPU_MANAGER_H
#define MPU_MANAGER_H

// Hack to prevent sensor_t collision with esp32-camera
#define sensor_t adafruit_sensor_t
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>
#undef sensor_t
#include <Wire.h>

// Interrupt flag - must be volatile for ISR
volatile bool _mpuDataReady = false;

// Interrupt Service Routine (ISR) - stored in fast RAM
void IRAM_ATTR mpuISR() {
    _mpuDataReady = true;
}

class MPUManager {
public:
    MPUManager() : _initialized(false), _readIndex(0), _total(0.0), _average(0.0) {
        for (int i = 0; i < _windowSize; i++) _readings[i] = 0.0;
    }

    // Call this in setup()
    bool begin(int intPin = 45) {
        // Pins 47 (SDA) and 21 (SCL) avoid PSRAM/USB/Radar conflicts
        if (!Wire.begin(47, 21)) return false; 
        if (!_mpu.begin()) return false;

        // Bike-optimized settings
        _mpu.setAccelerometerRange(MPU6050_RANGE_4_G);
        _mpu.setFilterBandwidth(MPU6050_BAND_21_HZ);

        // Configure Hardware Interrupt on Pin 45
        pinMode(intPin, INPUT_PULLUP);
        attachInterrupt(digitalPinToInterrupt(intPin), mpuISR, FALLING);

        _initialized = true;
        return true;
    }

    // Call this in loop() - it returns the current braking state
    bool isBraking() {
        if (!_initialized) return false;

        // Only update math if the INT pin signaled new data
        if (_mpuDataReady) {
            _mpuDataReady = false; // Clear flag
            
            sensors_event_t a, g, temp;
            _mpu.getEvent(&a, &g, &temp);

            // Moving Average Filter
            _total = _total - _readings[_readIndex];
            _readings[_readIndex] = a.acceleration.y; // Monitor Forward/Backward
            _total = _total + _readings[_readIndex];
            _readIndex = (_readIndex + 1) % _windowSize;
            _average = _total / _windowSize;

            // Deceleration threshold for a bike
            _lastBrakeState = (_average < -1.8);
        }

        return _lastBrakeState;
    }

private:
    Adafruit_MPU6050 _mpu;
    bool _initialized;
    bool _lastBrakeState = false;
    static const int _windowSize = 10; 
    float _readings[_windowSize];
    int _readIndex;
    float _total;
    float _average;
};

#endif