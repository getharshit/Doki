/**
 * @file blank_app.h
 * @brief Dance Image Screensaver - Manual Byte Swap Fix
 * 
 * This version creates a corrected copy of the image with swapped bytes
 */

#ifndef BLANK_APP_H
#define BLANK_APP_H

#include "doki/app_base.h"
#include "assets/dance.h"

class BlankApp : public Doki::DokiApp {
public:
    BlankApp() : DokiApp("blank", "Screensaver") {
        _correctedImage = nullptr;
    }
    
    void onCreate() override {
        log("Creating Dance Image Screensaver (with byte swap)...");
        
        // Black background
        lv_obj_set_style_bg_color(getScreen(), lv_color_hex(0x000000), 0);
        
        // Get original image data
        const lv_img_dsc_t* original = &dance;
        
        log(("Original image: " + String(original->header.w) + "x" + String(original->header.h)).c_str());
        log(("Data size: " + String(original->data_size) + " bytes").c_str());
        log(("Color format: " + String(original->header.cf)).c_str());
        
        // Create corrected image in PSRAM
        _correctedImage = (lv_img_dsc_t*)ps_malloc(sizeof(lv_img_dsc_t));
        if (!_correctedImage) {
            log("ERROR: Failed to allocate corrected image descriptor!");
            return;
        }
        
        // Copy header
        memcpy(_correctedImage, original, sizeof(lv_img_dsc_t));
        
        // Allocate new data buffer in PSRAM
        uint8_t* newData = (uint8_t*)ps_malloc(original->data_size);
        if (!newData) {
            log("ERROR: Failed to allocate data buffer!");
            free(_correctedImage);
            _correctedImage = nullptr;
            return;
        }
        
        // Swap bytes for RGB565
        const uint8_t* srcData = (const uint8_t*)original->data;
        
        log("Swapping RGB565 bytes...");
        
        // RGB565A8 format: 2 bytes per pixel (RGB565) + 1 byte alpha
        // We need to swap the 2 RGB bytes
        size_t pixelCount = original->header.w * original->header.h;
        
        if (original->header.cf == LV_IMG_CF_RGB565A8) {
            // RGB565A8: 2 bytes RGB + 1 byte alpha per pixel
            for (size_t i = 0; i < pixelCount; i++) {
                size_t offset = i * 3;
                // Swap RGB bytes
                newData[offset + 0] = srcData[offset + 1];  // Swap byte 0 and 1
                newData[offset + 1] = srcData[offset + 0];
                newData[offset + 2] = srcData[offset + 2];  // Keep alpha
            }
        } else if (original->header.cf == LV_IMG_CF_TRUE_COLOR || 
                   original->header.cf == LV_IMG_CF_TRUE_COLOR_ALPHA) {
            // True color: swap R and B channels
            size_t bytesPerPixel = (original->header.cf == LV_IMG_CF_TRUE_COLOR) ? 3 : 4;
            for (size_t i = 0; i < pixelCount; i++) {
                size_t offset = i * bytesPerPixel;
                newData[offset + 0] = srcData[offset + 2];  // B -> R
                newData[offset + 1] = srcData[offset + 1];  // G stays
                newData[offset + 2] = srcData[offset + 0];  // R -> B
                if (bytesPerPixel == 4) {
                    newData[offset + 3] = srcData[offset + 3];  // Keep alpha
                }
            }
        } else {
            // Unknown format, just copy
            log("Unknown format, copying as-is");
            memcpy(newData, srcData, original->data_size);
        }
        
        _correctedImage->data = newData;
        
        log("✓ Byte swap complete!");
        
        // Create image object
        _image = lv_img_create(getScreen());
        
        // Use corrected image
        lv_img_set_src(_image, _correctedImage);
        
        // Center the image
        lv_obj_center(_image);
        
        // Anti-aliasing
        lv_img_set_antialias(_image, true);
        
        // Fade in
        lv_obj_set_style_opa(getScreen(), LV_OPA_0, 0);
        lv_obj_fade_in(getScreen(), 500, 0);
        
        log("✓ Dance image loaded with correct colors!");
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
        
        // Free corrected image data
        if (_correctedImage) {
            if (_correctedImage->data) {
                free((void*)_correctedImage->data);
            }
            free(_correctedImage);
            _correctedImage = nullptr;
            log("Corrected image memory freed");
        }
    }

private:
    lv_obj_t* _image;
    lv_img_dsc_t* _correctedImage;
};

#endif // BLANK_APP_H
