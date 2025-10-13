/**
 * @file goodbye_app.h
 * @brief Simple goodbye app for testing app switching
 */

#ifndef GOODBYE_APP_H
#define GOODBYE_APP_H

#include "doki/app_base.h"

class GoodbyeApp : public Doki::DokiApp {
public:
    GoodbyeApp() : DokiApp("goodbye", "Goodbye App") {}
    
    void onCreate() override {
        log("Creating Goodbye App UI...");
        
        // Create main label
        _label = lv_label_create(getScreen());
        lv_label_set_text(_label, "Goodbye Doki OS!");
        lv_obj_align(_label, LV_ALIGN_CENTER, 0, -40);
        lv_obj_set_style_text_font(_label, &lv_font_montserrat_24, 0);
        lv_obj_set_style_text_color(_label, lv_color_hex(0xFF6B6B), 0);  // Red color
        
        // Create subtitle
        _subtitle = lv_label_create(getScreen());
        lv_label_set_text(_subtitle, "See you later!");
        lv_obj_align(_subtitle, LV_ALIGN_CENTER, 0, 0);
        lv_obj_set_style_text_color(_subtitle, lv_color_hex(0xFF6B6B), 0);
        
        // Create uptime label - start at 0
        _uptimeLabel = lv_label_create(getScreen());
        lv_label_set_text(_uptimeLabel, "Uptime: 0s");
        lv_obj_align(_uptimeLabel, LV_ALIGN_CENTER, 0, 40);
        
        _lastUpdate = 0;  // Force update on first call
    }
    
    void onStart() override {
        log("Goodbye App started!");
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
        log("Goodbye App paused");
    }
    
    void onDestroy() override {
        log("Goodbye App destroyed");
        // LVGL auto-cleans UI elements
    }

private:
    lv_obj_t* _label;
    lv_obj_t* _subtitle;
    lv_obj_t* _uptimeLabel;
    uint32_t _lastUpdate;
};

#endif // GOODBYE_APP_H