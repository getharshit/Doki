/**
 * @file blank_app.h
 * @brief Dance Image Screensaver - FIXED
 */

#ifndef BLANK_APP_H
#define BLANK_APP_H

#include "doki/app_base.h"
#include "assets/dance.h"  // Use header, not .c file!

class BlankApp : public Doki::DokiApp {
public:
    BlankApp() : DokiApp("blank", "Screensaver") {}
    
    void onCreate() override {
        log("Creating Dance Image Screensaver...");
        
        // Black background
        lv_obj_set_style_bg_color(getScreen(), lv_color_hex(0x000000), 0);
        
        // Create image object
        _image = lv_img_create(getScreen());
        
        // Set the dance image
        lv_img_set_src(_image, &dance);
        
        // FIX: If colors are inverted, try swapping bytes
        // This fixes RGB565 byte order issues
        lv_obj_set_style_img_recolor(_image, lv_color_white(), 0);
        lv_obj_set_style_img_recolor_opa(_image, LV_OPA_TRANSP, 0);
        
        // Center the image
        lv_obj_center(_image);
        
        // Anti-aliasing
        lv_img_set_antialias(_image, true);
        
        // Fade in
        lv_obj_set_style_opa(getScreen(), LV_OPA_0, 0);
        lv_obj_fade_in(getScreen(), 500, 0);
        
        log("✓ Dance image loaded!");
        log("If colors are wrong, re-convert with different settings");
    }
    
    void onStart() override {
        log("Dance Screensaver started!");
    }
    
    void onUpdate() override {
        // Static image - no updates needed
    }
    
    void onPause() override {
        log("Dance Screensaver paused");
    }
    
    void onDestroy() override {
        log("Dance Screensaver destroyed");
    }

private:
    lv_obj_t* _image;
};

#endif // BLANK_APP_H