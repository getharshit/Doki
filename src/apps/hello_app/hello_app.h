/**
 * @file hello_app.h
 * @brief Simple test app to demonstrate DokiApp usage
 * 
 * This is a minimal example showing how to create a Doki OS app.
 * Place this in: src/apps/hello_app/hello_app.h
 */

#ifndef HELLO_APP_H
#define HELLO_APP_H

#include "doki/app_base.h"

class HelloApp : public Doki::DokiApp {
public:
    HelloApp() : DokiApp("hello", "Hello App") {}
    
    void onCreate() override {
        log("Creating Hello App UI...");
        
        // Create a simple label
        _label = lv_label_create(getScreen());
        lv_label_set_text(_label, "Hello Doki OS!");
        lv_obj_align(_label, LV_ALIGN_CENTER, 0, -20);
        lv_obj_set_style_text_font(_label, &lv_font_montserrat_24, 0);
        
        // Create uptime label
        _uptimeLabel = lv_label_create(getScreen());
        lv_label_set_text(_uptimeLabel, "Uptime: 0s");
        lv_obj_align(_uptimeLabel, LV_ALIGN_CENTER, 0, 20);
        
        _lastUpdate = millis();
    }
    
    void onStart() override {
        log("Hello App started!");
    }
    
    void onUpdate() override {
        // Update every 1 second
        uint32_t now = millis();
        if (now - _lastUpdate >= 1000) {
            uint32_t uptime = getUptime() / 1000;  // Convert to seconds
            char buf[32];
            snprintf(buf, sizeof(buf), "Uptime: %lus", uptime);
            lv_label_set_text(_uptimeLabel, buf);
            _lastUpdate = now;
        }
    }
    
    void onPause() override {
        log("Hello App paused");
    }
    
    void onDestroy() override {
        log("Hello App destroyed");
        // LVGL auto-cleans UI elements
    }

private:
    lv_obj_t* _label;
    lv_obj_t* _uptimeLabel;
    uint32_t _lastUpdate;
};

#endif // HELLO_APP_H