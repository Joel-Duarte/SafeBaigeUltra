#ifndef FILTER_MODULE_H
#define FILTER_MODULE_H

class SignalFilter {
private:
    float _values[5]; 
    const float _alpha = 0.18f; // Smoothing factor (Adjust between 0.05 and 0.3)

public:
    SignalFilter() {
        for(int i = 0; i < 5; i++) _values[i] = -1.0f; 
    }

    // Renamed to 'update' to fix the compiler error
    float smooth(int index, float raw) {
        // If it's a new target, snap immediately to the value
        if (_values[index] < 0) {
            _values[index] = raw;
        } else {
            // Native FPU handles this EMA calculation
            _values[index] = (_alpha * raw) + (1.0f - _alpha) * _values[index];
        }
        return _values[index];
    }

    void reset(int index) {
        _values[index] = -1.0f;
    }
};

#endif