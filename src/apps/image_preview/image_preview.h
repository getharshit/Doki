/**
 * @file image_preview.h
 * @brief Static Image Preview App for Doki OS
 *
 * Displays static PNG/JPEG images loaded from SPIFFS.
 * Each display has its own image slot.
 */

#ifndef IMAGE_PREVIEW_H
#define IMAGE_PREVIEW_H

#include "doki/app_base.h"
#include "doki/media_service.h"

class ImagePreviewApp : public Doki::DokiApp {
public:
    /**
     * @brief Constructor
     */
    ImagePreviewApp()
        : DokiApp("image", "Image Preview"),
          _image(nullptr),
          _placeholderLabel(nullptr) {}

    void onCreate() override {
        log("Creating Image Preview App...");

        // Get display ID for this app
        uint8_t displayId = getDisplayId();

        // Get screen reference
        lv_obj_t* screen = getScreen();

        // Set black background
        lv_obj_set_style_bg_color(screen, lv_color_hex(0x000000), 0);

        // Check for PNG image first
        Doki::MediaInfo info = Doki::MediaService::getMediaInfo(displayId, Doki::MediaType::IMAGE_PNG);

        // If no PNG, check for JPEG
        if (!info.exists) {
            info = Doki::MediaService::getMediaInfo(displayId, Doki::MediaType::IMAGE_JPEG);
        }

        if (!info.exists) {
            showPlaceholder("No image uploaded\n\nUpload via dashboard");
            log("No image found for this display");
            return;
        }

        String logMsg = "Loading image from: " + info.path;
        log(logMsg.c_str());

        // Create image object
        _image = lv_img_create(screen);
        if (_image == nullptr) {
            log("Error: Failed to create image object");
            showPlaceholder("Error creating image");
            return;
        }

        // Set image source from SPIFFS path
        // LVGL requires "S:" prefix for SPIFFS driver
        String lvglPath = "S:" + info.path;
        lv_img_set_src(_image, lvglPath.c_str());

        // Check if image loaded successfully
        const void* src = lv_img_get_src(_image);
        if (src == nullptr) {
            log("Error: Failed to load image from SPIFFS");
            showPlaceholder("Error loading image");
            lv_obj_del(_image);
            _image = nullptr;
            return;
        }

        // Center the image
        lv_obj_center(_image);

        // Enable anti-aliasing for better quality
        lv_img_set_antialias(_image, true);

        // Optional: Add subtle zoom/fade animation
        // addZoomAnimation();

        log("✓ Image loaded successfully!");
        Serial.printf("[ImagePreview] Image size: %zu bytes\n", info.fileSize);
    }

    void onStart() override {
        log("Image Preview started!");
    }

    void onUpdate() override {
        // Static image - no updates needed
        // Could add periodic refresh check here if needed
    }

    void onPause() override {
        log("Image Preview paused");
    }

    void onDestroy() override {
        log("Image Preview destroyed");
        // LVGL auto-cleans image resources
        _image = nullptr;
        _placeholderLabel = nullptr;
    }

private:
    lv_obj_t* _image;                ///< LVGL image object
    lv_obj_t* _placeholderLabel;     ///< Placeholder text when no image

    /**
     * @brief Show placeholder text when no image is available
     * @param message Message to display
     */
    void showPlaceholder(const char* message) {
        // Black background
        lv_obj_set_style_bg_color(getScreen(), lv_color_hex(0x000000), 0);

        // Create centered label
        _placeholderLabel = lv_label_create(getScreen());
        lv_label_set_text(_placeholderLabel, message);
        lv_obj_set_style_text_color(_placeholderLabel, lv_color_hex(0x888888), 0);
        lv_obj_set_style_text_font(_placeholderLabel, &lv_font_montserrat_16, 0);
        lv_obj_set_style_text_align(_placeholderLabel, LV_TEXT_ALIGN_CENTER, 0);
        lv_obj_align(_placeholderLabel, LV_ALIGN_CENTER, 0, 0);
        lv_obj_set_width(_placeholderLabel, 200);
    }

    /**
     * @brief Add subtle zoom animation (optional)
     */
    void addZoomAnimation() {
        if (_image == nullptr) return;

        lv_anim_t anim;
        lv_anim_init(&anim);
        lv_anim_set_var(&anim, _image);
        lv_anim_set_time(&anim, 3000);
        lv_anim_set_values(&anim, 256, 280);  // Zoom from 100% to 109%
        lv_anim_set_path_cb(&anim, lv_anim_path_ease_in_out);
        lv_anim_set_repeat_count(&anim, LV_ANIM_REPEAT_INFINITE);
        lv_anim_set_playback_time(&anim, 3000);
        lv_anim_set_exec_cb(&anim, [](void* obj, int32_t v) {
            lv_img_set_zoom((lv_obj_t*)obj, v);
        });
        lv_anim_start(&anim);
    }
};

#endif // IMAGE_PREVIEW_H
