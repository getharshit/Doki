/**
 * @file blank_app.h
 * @brief OPTIMIZED Screensaver with manual animations (high performance)
 * 
 * Changes from original:
 * - Removed ALL LVGL animations (they're expensive)
 * - Using manual position/opacity updates (much faster)
 * - Reduced from 11 animations to 3 simple moving circles
 * - Update at 20 FPS instead of 60 FPS
 */

#ifndef BLANK_APP_H
#define BLANK_APP_H

#include "doki/app_base.h"
#include <math.h>

class BlankApp : public Doki::DokiApp {
public:
    BlankApp() : DokiApp("blank", "Screensaver") {}
    
    void onCreate() override {
        log("Creating OPTIMIZED Screensaver...");
        
        // Black background
        lv_obj_set_style_bg_color(getScreen(), lv_color_hex(0x000000), 0);
        
        // Create only 3 simple circles (reduced from complex multi-object setup)
        for (int i = 0; i < 3; i++) {
            _circles[i] = lv_obj_create(getScreen());
            
            // Size (store for bounds checking)
            _circleState[i].size = 60 - (i * 15);
            lv_obj_set_size(_circles[i], _circleState[i].size, _circleState[i].size);
            
            // Make them circles
            lv_obj_set_style_radius(_circles[i], LV_RADIUS_CIRCLE, 0);
            lv_obj_set_style_border_width(_circles[i], 0, 0);
            
            // Different colors with low opacity
            uint32_t colors[] = {0x667eea, 0x764ba2, 0x00d4ff};
            lv_obj_set_style_bg_color(_circles[i], lv_color_hex(colors[i]), 0);
            lv_obj_set_style_bg_opa(_circles[i], LV_OPA_30, 0);
            
            // Start at center (screen is 240x320)
            _circleState[i].x = 120;  // Center X (240/2)
            _circleState[i].y = 160;  // Center Y (320/2)
            
            // Set initial position
            lv_obj_set_pos(_circles[i], 
                          (int16_t)(_circleState[i].x - _circleState[i].size / 2),
                          (int16_t)(_circleState[i].y - _circleState[i].size / 2));
            
            // Initialize animation state with slower velocities
            _circleState[i].vx = 0.5f + (i * 0.2f);   // X velocity
            _circleState[i].vy = 0.6f + (i * 0.25f);  // Y velocity
            _circleState[i].phase = i * 2.0f;         // Phase offset
        }
        
        // Simple logo (no animation)
        _logoLabel = lv_label_create(getScreen());
        lv_label_set_text(_logoLabel, "DOKI OS");
        lv_obj_align(_logoLabel, LV_ALIGN_BOTTOM_RIGHT, -15, -15);
        lv_obj_set_style_text_font(_logoLabel, &lv_font_montserrat_14, 0);
        lv_obj_set_style_text_color(_logoLabel, lv_color_hex(0x667eea), 0);
        lv_obj_set_style_opa(_logoLabel, LV_OPA_30, 0);
        
        _lastUpdate = 0;
        _animPhase = 0;
    }
    
    void onStart() override {
        log("Screensaver started!");
        _lastUpdate = millis();
    }
    
    void onUpdate() override {
        uint32_t now = millis();
        
        // Update at 20 FPS (50ms interval) - much more reasonable than 60 FPS
        if (now - _lastUpdate < 50) {
            return;  // Skip this frame
        }
        
        float deltaTime = (now - _lastUpdate) / 1000.0f;
        _lastUpdate = now;
        
        // Update animation phase
        _animPhase += deltaTime * 2.0f;  // Slow rotation
        if (_animPhase > 6.28f) _animPhase -= 6.28f;
        
        // Manually update each circle (NO LVGL ANIMATIONS)
        for (int i = 0; i < 3; i++) {
            CircleState* s = &_circleState[i];
            int halfSize = s->size / 2;
            
            // Update position
            s->x += s->vx;
            s->y += s->vy;
            
            // Bounce off edges - proper bounds checking
            // Screen: 240 wide x 320 tall
            // Circle position is CENTER, so we need to account for radius
            if (s->x - halfSize < 0) {
                s->x = halfSize;
                s->vx = -s->vx;
            } else if (s->x + halfSize > 240) {
                s->x = 240 - halfSize;
                s->vx = -s->vx;
            }
            
            if (s->y - halfSize < 0) {
                s->y = halfSize;
                s->vy = -s->vy;
            } else if (s->y + halfSize > 320) {
                s->y = 320 - halfSize;
                s->vy = -s->vy;
            }
            
            // Update LVGL object position
            // LVGL position is TOP-LEFT corner, so subtract half size
            lv_obj_set_pos(_circles[i], 
                          (int16_t)(s->x - halfSize),
                          (int16_t)(s->y - halfSize));
            
            // Simple opacity pulse (using sine wave)
            uint8_t opa = 20 + (uint8_t)(15 * sin(_animPhase + s->phase));
            lv_obj_set_style_bg_opa(_circles[i], opa, 0);
        }
    }
    
    void onPause() override {
        log("Screensaver paused");
        // No animations to stop - we're doing manual updates
    }
    
    void onDestroy() override {
        log("Screensaver destroyed");
        // LVGL auto-deletes objects
    }

private:
    // Simple state tracking for each circle
    struct CircleState {
        float x, y;      // Position (center of circle)
        float vx, vy;    // Velocity
        float phase;     // Animation phase offset
        int size;        // Circle diameter (NEW - needed for bounds checking)
        
        CircleState() : x(0), y(0), vx(0), vy(0), phase(0), size(0) {}
    };
    
    lv_obj_t* _circles[3];
    CircleState _circleState[3];
    lv_obj_t* _logoLabel;
    
    uint32_t _lastUpdate;
    float _animPhase;
};

#endif // BLANK_APP_H