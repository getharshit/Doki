/**
 * @file blank_app.h
 * @brief Simple Image Screensaver (No Manual Byte Swapping)
 * 
 * IMPORTANT: If colors are wrong, reconvert your image with:
 * - LVGL Image Converter (https://lvgl.io/tools/imageconverter)
 * - Color format: CF_TRUE_COLOR_ALPHA
 * - Enable "Swap bytes" option
 * 
 * This will fix RGB/BGR color issues at conversion time.
 */

#ifndef BLANK_APP_H
#define BLANK_APP_H

#include "doki/app_base.h"
#include "assets/dance.h"

class BlankApp : public Doki::DokiApp {
public:
    BlankApp() : DokiApp("blank", "Screensaver") {}
    
    void onCreate() override {
        log("Creating Image Screensaver...");
        
        // Black background
        lv_obj_set_style_bg_color(getScreen(), lv_color_hex(0x000000), 0);
        
        // Create image object
        _image = lv_img_create(getScreen());
        
        // Set image source directly - no manual byte swapping!
        lv_img_set_src(_image, &dance);
        
        // Center the image
        lv_obj_center(_image);
        
        // Enable anti-aliasing for better quality
        lv_img_set_antialias(_image, true);
        
        // Optional: Add subtle zoom animation
        // addZoomAnimation();
        
        log("✓ Image loaded successfully!");
    }
    
    void onStart() override {
        log("Screensaver started!");
    }
    
    void onUpdate() override {
        // Static image - no updates needed
        // If you want animation, uncomment addZoomAnimation() above
    }
    
    void onPause() override {
        log("Screensaver paused");
    }
    
    void onDestroy() override {
        log("Screensaver destroyed");
        // LVGL auto-cleans image resources
    }

private:
    lv_obj_t* _image;
    
    // Optional: Add subtle zoom animation
    void addZoomAnimation() {
        lv_anim_t anim;
        lv_anim_init(&anim);
        lv_anim_set_var(&anim, _image);
        lv_anim_set_time(&anim, 3000);
        lv_anim_set_values(&anim, 256, 300);  // Zoom from 100% to 117%
        lv_anim_set_path_cb(&anim, lv_anim_path_ease_in_out);
        lv_anim_set_repeat_count(&anim, LV_ANIM_REPEAT_INFINITE);
        lv_anim_set_playback_time(&anim, 3000);
        lv_anim_set_exec_cb(&anim, [](void* obj, int32_t v) {
            lv_img_set_zoom((lv_obj_t*)obj, v);
        });
        lv_anim_start(&anim);
    }
};

#endif // BLANK_APP_H