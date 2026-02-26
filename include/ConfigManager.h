#ifndef CONFIG_MANAGER_H
#define CONFIG_MANAGER_H

#include <Arduino.h>
#include <Preferences.h>

// Declare your existing globals
extern uint8_t cfg_max_dist;
extern uint8_t cfg_direction;
extern uint8_t cfg_min_speed;
extern uint8_t cfg_delay_time;
extern uint8_t cfg_trigger_acc;
extern uint8_t cfg_snr_limit;
extern uint8_t cfg_rapid_threshold;
extern uint32_t cameraTimerMs;

class ConfigManager {
public:
    void load() {
        Preferences preferences;
        preferences.begin("radar", true);

        cfg_max_dist       = preferences.getUChar("max_dist", cfg_max_dist);
        cfg_direction      = preferences.getUChar("direction", cfg_direction);
        cfg_min_speed      = preferences.getUChar("min_speed", cfg_min_speed);
        cfg_delay_time     = preferences.getUChar("delay_time", cfg_delay_time);
        cfg_trigger_acc    = preferences.getUChar("trigger_acc", cfg_trigger_acc);
        cfg_snr_limit      = preferences.getUChar("snr_limit", cfg_snr_limit);
        cfg_rapid_threshold= preferences.getUChar("rapid_th", cfg_rapid_threshold);
        cameraTimerMs      = preferences.getUInt("cam_timer", cameraTimerMs);

        preferences.end();
    }

    void save() {
        Preferences preferences;
        preferences.begin("radar", false);

        preferences.putUChar("max_dist", cfg_max_dist);
        preferences.putUChar("direction", cfg_direction);
        preferences.putUChar("min_speed", cfg_min_speed);
        preferences.putUChar("delay_time", cfg_delay_time);
        preferences.putUChar("trigger_acc", cfg_trigger_acc);
        preferences.putUChar("snr_limit", cfg_snr_limit);
        preferences.putUChar("rapid_th", cfg_rapid_threshold);
        preferences.putUInt("cam_timer", cameraTimerMs);

        preferences.end();
    }

    void factoryReset() {
        Preferences preferences;
        preferences.begin("radar", false);
        preferences.clear();
        preferences.end();
    }
};

#endif