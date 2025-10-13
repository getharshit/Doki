/**
 * @file blank_app.h
 * @brief Blank/Screensaver App - Minimal display for Doki OS
 */

#ifndef BLANK_APP_H
#define BLANK_APP_H

#include "doki/app_base.h"

class BlankApp : public Doki::DokiApp {
public:
    BlankApp() : DokiApp("blank", "Screensaver") {}
    
    void onCreate() override {
        log("Creating Blank App UI...");
        
        // Create a subtle "Doki OS" label in the corner
        _logoLabel = lv_label_create(getScreen());
        lv_label_set_text(_logoLabel, "Doki OS");
        lv_obj_align(_logoLabel, LV_ALIGN_BOTTOM_RIGHT, -10, -10);
        lv_obj_set_style_text_font(_logoLabel, &lv_font_montserrat_12, 0);
        lv_obj_set_style_text_color(_logoLabel, lv_color_hex(0x333333), 0);
        lv_obj_set_style_opa(_logoLabel, LV_OPA_30, 0);  // Very subtle
        
        // Optional: Add a bouncing dot animation
        _dotX = 240 / 2;  // TFT_WIDTH / 2
        _dotY = 320 / 2;  // TFT_HEIGHT / 2
        _dotVelX = 2;
        _dotVelY = 2;
        
        _dot = lv_obj_create(getScreen());
        lv_obj_set_size(_dot, 8, 8);
        lv_obj_set_style_radius(_dot, LV_RADIUS_CIRCLE, 0);
        lv_obj_set_style_bg_color(_dot, lv_color_hex(0x667eea), 0);
        lv_obj_set_style_border_width(_dot, 0, 0);
        lv_obj_set_pos(_dot, _dotX, _dotY);
        
        _lastUpdate = 0;
    }
    
    void onStart() override {
        log("Blank App started - Low power mode");
    }
    
    void onUpdate() override {
        // Update bouncing dot animation at 30 FPS
        uint32_t now = millis();
        if (now - _lastUpdate >= 33) {  // ~30 FPS
            animateDot();
            _lastUpdate = now;
        }
    }
    
    void onPause() override {
        log("Blank App paused");
    }
    
    void onDestroy() override {
        log("Blank App destroyed");
    }

private:
    lv_obj_t* _logoLabel;
    lv_obj_t* _dot;
    
    int16_t _dotX;
    int16_t _dotY;
    int8_t _dotVelX;
    int8_t _dotVelY;
    
    uint32_t _lastUpdate;
    
    void animateDot() {
        // Update position
        _dotX += _dotVelX;
        _dotY += _dotVelY;
        
        // Bounce off edges
        if (_dotX <= 0 || _dotX >= 240 - 8) {
            _dotVelX = -_dotVelX;
            _dotX = constrain(_dotX, 0, 240 - 8);
        }
        
        if (_dotY <= 0 || _dotY >= 320 - 8) {
            _dotVelY = -_dotVelY;
            _dotY = constrain(_dotY, 0, 320 - 8);
        }
        
        // Update position
        lv_obj_set_pos(_dot, _dotX, _dotY);
    }
};

#endif // BLANK_APP_H