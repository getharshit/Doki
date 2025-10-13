/**
 * @file blank_app.h
 * @brief Enhanced Blank/Screensaver App with bouncing logo and trails
 */

#ifndef BLANK_APP_H
#define BLANK_APP_H

#include "doki/app_base.h"

class BlankApp : public Doki::DokiApp {
public:
    BlankApp() : DokiApp("blank", "Screensaver") {}
    
    void onCreate() override {
        log("Creating Enhanced Blank App...");
        
        // Black background
        lv_obj_set_style_bg_color(getScreen(), lv_color_hex(0x000000), 0);
        
        // Create trail effect (multiple fading circles)
        for (int i = 0; i < 5; i++) {
            _trail[i] = lv_obj_create(getScreen());
            lv_obj_set_size(_trail[i], 20 - i * 3, 20 - i * 3);
            lv_obj_set_style_radius(_trail[i], LV_RADIUS_CIRCLE, 0);
            lv_obj_set_style_bg_color(_trail[i], lv_color_hex(0x667eea), 0);
            lv_obj_set_style_bg_opa(_trail[i], 50 - i * 10, 0);
            lv_obj_set_style_border_width(_trail[i], 0, 0);
            lv_obj_add_flag(_trail[i], LV_OBJ_FLAG_HIDDEN);
        }
        
        // Main bouncing dot
        _dot = lv_obj_create(getScreen());
        lv_obj_set_size(_dot, 20, 20);
        lv_obj_set_style_radius(_dot, LV_RADIUS_CIRCLE, 0);
        lv_obj_set_style_bg_color(_dot, lv_color_hex(0x667eea), 0);
        lv_obj_set_style_border_width(_dot, 2, 0);
        lv_obj_set_style_border_color(_dot, lv_color_hex(0x764ba2), 0);
        lv_obj_set_style_shadow_width(_dot, 10, 0);
        lv_obj_set_style_shadow_color(_dot, lv_color_hex(0x667eea), 0);
        lv_obj_set_style_shadow_opa(_dot, LV_OPA_50, 0);
        
        // "Doki OS" subtle label
        _logoLabel = lv_label_create(getScreen());
        lv_label_set_text(_logoLabel, "DOKI OS");
        lv_obj_align(_logoLabel, LV_ALIGN_BOTTOM_RIGHT, -15, -15);
        lv_obj_set_style_text_font(_logoLabel, &lv_font_montserrat_12, 0);
        lv_obj_set_style_text_color(_logoLabel, lv_color_hex(0x333333), 0);
        lv_obj_set_style_opa(_logoLabel, LV_OPA_20, 0);
        
        // Initialize position and velocity
        _dotX = 120;
        _dotY = 160;
        _dotVelX = 3.2f;
        _dotVelY = 2.7f;
        
        // Trail history
        for (int i = 0; i < 5; i++) {
            _trailX[i] = _dotX;
            _trailY[i] = _dotY;
        }
        
        _lastUpdate = 0;
        _colorPhase = 0;
        
        lv_obj_set_pos(_dot, _dotX, _dotY);
        
        // Fade in animation
        lv_obj_set_style_opa(getScreen(), LV_OPA_0, 0);
        lv_obj_fade_in(getScreen(), 500, 0);
    }
    
    void onStart() override {
        log("Blank App started - Screensaver mode");
    }
    
    void onUpdate() override {
        uint32_t now = millis();
        
        // Update at 30 FPS (instead of 60) for better dual-display performance
        if (now - _lastUpdate >= 33) {
            animateDot();
            updateTrail();
            animateColor();
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
    lv_obj_t* _dot;
    lv_obj_t* _trail[5];
    lv_obj_t* _logoLabel;
    
    float _dotX;
    float _dotY;
    float _dotVelX;
    float _dotVelY;
    
    float _trailX[5];
    float _trailY[5];
    
    uint32_t _lastUpdate;
    float _colorPhase;
    
    void animateDot() {
        // Update position
        _dotX += _dotVelX;
        _dotY += _dotVelY;
        
        // Bounce off edges with slight randomization
        if (_dotX <= 0 || _dotX >= 220) {
            _dotVelX = -_dotVelX;
            _dotX = constrain(_dotX, 0, 220);
            // Add slight randomization to velocity
            _dotVelX *= (0.95f + (random(0, 100) / 1000.0f));
        }
        
        if (_dotY <= 0 || _dotY >= 300) {
            _dotVelY = -_dotVelY;
            _dotY = constrain(_dotY, 0, 300);
            _dotVelY *= (0.95f + (random(0, 100) / 1000.0f));
        }
        
        // Update dot position
        lv_obj_set_pos(_dot, (int16_t)_dotX, (int16_t)_dotY);
    }
    
    void updateTrail() {
        // Shift trail positions
        for (int i = 4; i > 0; i--) {
            _trailX[i] = _trailX[i-1];
            _trailY[i] = _trailY[i-1];
        }
        _trailX[0] = _dotX;
        _trailY[0] = _dotY;
        
        // Update trail object positions
        for (int i = 0; i < 5; i++) {
            lv_obj_clear_flag(_trail[i], LV_OBJ_FLAG_HIDDEN);
            lv_obj_set_pos(_trail[i], (int16_t)_trailX[i], (int16_t)_trailY[i]);
        }
    }
    
    void animateColor() {
        // Cycle through colors slowly
        _colorPhase += 0.01f;
        if (_colorPhase > 6.28f) _colorPhase = 0;
        
        // HSV to RGB-like color cycling
        uint8_t r = 102 + (uint8_t)(50 * sin(_colorPhase));
        uint8_t g = 126 + (uint8_t)(50 * sin(_colorPhase + 2.09f));
        uint8_t b = 234 + (uint8_t)(20 * sin(_colorPhase + 4.19f));
        
        uint32_t color = (r << 16) | (g << 8) | b;
        lv_obj_set_style_bg_color(_dot, lv_color_hex(color), 0);
        lv_obj_set_style_border_color(_dot, lv_color_hex(color), 0);
        lv_obj_set_style_shadow_color(_dot, lv_color_hex(color), 0);
        
        // Update trail colors with fade
        for (int i = 0; i < 5; i++) {
            lv_obj_set_style_bg_color(_trail[i], lv_color_hex(color), 0);
        }
    }
};

#endif // BLANK_APP_H