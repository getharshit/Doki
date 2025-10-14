/**
 * @file blank_app.h
 * @brief Screensaver using LVGL's built-in animation system
 * Uses LVGL native animations for smooth, hardware-optimized performance
 */

#ifndef BLANK_APP_H
#define BLANK_APP_H

#include "doki/app_base.h"

class BlankApp : public Doki::DokiApp {
public:
    BlankApp() : DokiApp("blank", "Screensaver") {}
    
    void onCreate() override {
        log("Creating LVGL Native Screensaver...");
        
        // Black background
        lv_obj_set_style_bg_color(getScreen(), lv_color_hex(0x000000), 0);
        
        // Create multiple animated circles with different speeds
        for (int i = 0; i < 3; i++) {
            _circles[i] = lv_obj_create(getScreen());
            
            // Size varies (80, 60, 40)
            int size = 80 - (i * 20);
            lv_obj_set_size(_circles[i], size, size);
            
            // Make them circles
            lv_obj_set_style_radius(_circles[i], LV_RADIUS_CIRCLE, 0);
            lv_obj_set_style_border_width(_circles[i], 0, 0);
            
            // Different colors
            uint32_t colors[] = {0x667eea, 0x764ba2, 0x00d4ff};
            lv_obj_set_style_bg_color(_circles[i], lv_color_hex(colors[i]), 0);
            lv_obj_set_style_bg_opa(_circles[i], LV_OPA_30 + (i * 20), 0);
            
            // Add shadow/glow effect
            lv_obj_set_style_shadow_width(_circles[i], 20, 0);
            lv_obj_set_style_shadow_color(_circles[i], lv_color_hex(colors[i]), 0);
            lv_obj_set_style_shadow_opa(_circles[i], LV_OPA_50, 0);
            
            // Start at center
            lv_obj_center(_circles[i]);
        }
        
        // Create animated arcs (spinning rings)
        for (int i = 0; i < 2; i++) {
            _arcs[i] = lv_arc_create(getScreen());
            
            // Size
            int size = 150 - (i * 40);
            lv_obj_set_size(_arcs[i], size, size);
            lv_obj_center(_arcs[i]);
            
            // Arc settings
            lv_arc_set_rotation(_arcs[i], 0);
            lv_arc_set_bg_angles(_arcs[i], 0, 360);
            lv_arc_set_angles(_arcs[i], 0, 120);
            
            // Style
            lv_obj_set_style_arc_width(_arcs[i], 4, LV_PART_MAIN);
            lv_obj_set_style_arc_width(_arcs[i], 4, LV_PART_INDICATOR);
            
            uint32_t colors[] = {0x667eea, 0x764ba2};
            lv_obj_set_style_arc_color(_arcs[i], lv_color_hex(colors[i]), LV_PART_INDICATOR);
            lv_obj_set_style_arc_opa(_arcs[i], LV_OPA_40, LV_PART_INDICATOR);
            
            // Hide background arc
            lv_obj_set_style_arc_opa(_arcs[i], LV_OPA_0, LV_PART_MAIN);
            
            // Remove knob
            lv_obj_set_style_bg_opa(_arcs[i], LV_OPA_0, LV_PART_KNOB);
        }
        
        // "DOKI OS" label
        _logoLabel = lv_label_create(getScreen());
        lv_label_set_text(_logoLabel, "DOKI OS");
        lv_obj_align(_logoLabel, LV_ALIGN_BOTTOM_RIGHT, -15, -15);
        lv_obj_set_style_text_font(_logoLabel, &lv_font_montserrat_14, 0);
        lv_obj_set_style_text_color(_logoLabel, lv_color_hex(0x667eea), 0);
        lv_obj_set_style_opa(_logoLabel, LV_OPA_30, 0);
        
        // Start animations
        startAnimations();
    }
    
    void onStart() override {
        log("Screensaver started!");
    }
    
    void onUpdate() override {
        // LVGL handles all animations automatically!
        // No manual updates needed
    }
    
    void onPause() override {
        log("Screensaver paused");
        // Stop animations
        for (int i = 0; i < 3; i++) {
            lv_anim_del(_circles[i], NULL);
        }
        for (int i = 0; i < 2; i++) {
            lv_anim_del(_arcs[i], NULL);
        }
    }
    
    void onDestroy() override {
        log("Screensaver destroyed");
        // Animations auto-deleted with objects
    }

private:
    lv_obj_t* _circles[3];
    lv_obj_t* _arcs[2];
    lv_obj_t* _logoLabel;
    
    // Animation callback for position
    static void anim_x_cb(void* obj, int32_t v) {
        lv_obj_set_x((lv_obj_t*)obj, v);
    }
    
    static void anim_y_cb(void* obj, int32_t v) {
        lv_obj_set_y((lv_obj_t*)obj, v);
    }
    
    // Animation callback for opacity
    static void anim_opa_cb(void* obj, int32_t v) {
        lv_obj_set_style_bg_opa((lv_obj_t*)obj, v, 0);
    }
    
    // Animation callback for arc rotation
    static void anim_arc_cb(void* obj, int32_t v) {
        lv_arc_set_rotation((lv_obj_t*)obj, v);
    }
    
    void startAnimations() {
        // Animate circles - smooth floating movement
        for (int i = 0; i < 3; i++) {
            // X position animation (left-right)
            lv_anim_t anim_x;
            lv_anim_init(&anim_x);
            lv_anim_set_var(&anim_x, _circles[i]);
            lv_anim_set_values(&anim_x, 20, 220 - (80 - i * 20));
            lv_anim_set_time(&anim_x, 4000 + (i * 1000));  // Different speeds
            lv_anim_set_exec_cb(&anim_x, anim_x_cb);
            lv_anim_set_path_cb(&anim_x, lv_anim_path_ease_in_out);
            lv_anim_set_repeat_count(&anim_x, LV_ANIM_REPEAT_INFINITE);
            lv_anim_set_playback_time(&anim_x, 4000 + (i * 1000));
            lv_anim_start(&anim_x);
            
            // Y position animation (up-down)
            lv_anim_t anim_y;
            lv_anim_init(&anim_y);
            lv_anim_set_var(&anim_y, _circles[i]);
            lv_anim_set_values(&anim_y, 20, 300 - (80 - i * 20));
            lv_anim_set_time(&anim_y, 5000 + (i * 1200));  // Different speeds
            lv_anim_set_exec_cb(&anim_y, anim_y_cb);
            lv_anim_set_path_cb(&anim_y, lv_anim_path_ease_in_out);
            lv_anim_set_repeat_count(&anim_y, LV_ANIM_REPEAT_INFINITE);
            lv_anim_set_playback_time(&anim_y, 5000 + (i * 1200));
            lv_anim_set_delay(&anim_y, i * 500);  // Stagger start
            lv_anim_start(&anim_y);
            
            // Opacity pulse
            lv_anim_t anim_opa;
            lv_anim_init(&anim_opa);
            lv_anim_set_var(&anim_opa, _circles[i]);
            lv_anim_set_values(&anim_opa, LV_OPA_20, LV_OPA_60);
            lv_anim_set_time(&anim_opa, 3000);
            lv_anim_set_exec_cb(&anim_opa, anim_opa_cb);
            lv_anim_set_path_cb(&anim_opa, lv_anim_path_ease_in_out);
            lv_anim_set_repeat_count(&anim_opa, LV_ANIM_REPEAT_INFINITE);
            lv_anim_set_playback_time(&anim_opa, 3000);
            lv_anim_start(&anim_opa);
        }
        
        // Animate arcs - spinning
        for (int i = 0; i < 2; i++) {
            lv_anim_t anim_arc;
            lv_anim_init(&anim_arc);
            lv_anim_set_var(&anim_arc, _arcs[i]);
            lv_anim_set_values(&anim_arc, 0, 360);
            lv_anim_set_time(&anim_arc, 3000 + (i * 1000));  // Different speeds
            lv_anim_set_exec_cb(&anim_arc, anim_arc_cb);
            lv_anim_set_repeat_count(&anim_arc, LV_ANIM_REPEAT_INFINITE);
            lv_anim_start(&anim_arc);
        }
        
        // Animate logo pulse
        lv_anim_t anim_logo;
        lv_anim_init(&anim_logo);
        lv_anim_set_var(&anim_logo, _logoLabel);
        lv_anim_set_values(&anim_logo, LV_OPA_10, LV_OPA_50);
        lv_anim_set_time(&anim_logo, 2000);
        lv_anim_set_exec_cb(&anim_logo, [](void* obj, int32_t v) {
            lv_obj_set_style_opa((lv_obj_t*)obj, v, 0);
        });
        lv_anim_set_path_cb(&anim_logo, lv_anim_path_ease_in_out);
        lv_anim_set_repeat_count(&anim_logo, LV_ANIM_REPEAT_INFINITE);
        lv_anim_set_playback_time(&anim_logo, 2000);
        lv_anim_start(&anim_logo);
    }
};

#endif // BLANK_APP_H