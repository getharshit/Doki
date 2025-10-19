/**
 * @file image_preview.h
 * @brief Static Image Preview App for Doki OS
 *
 * Displays static PNG/JPEG images loaded from MediaCache (PSRAM/LittleFS).
 * Each display has its own image slot.
 */

#ifndef IMAGE_PREVIEW_H
#define IMAGE_PREVIEW_H

#include "doki/app_base.h"
#include "doki/media_service.h"
#include "doki/media_cache.h"

class ImagePreviewApp : public Doki::DokiApp {
public:
    /**
     * @brief Constructor
     */
    ImagePreviewApp()
        : DokiApp("image", "Image Preview"),
          _image(nullptr),
          _placeholderLabel(nullptr),
          _imageData(nullptr),
          _imageSize(0) {}

    void onCreate() override {
        log("Creating Image Preview App...");

        // Get display ID for this app
        uint8_t displayId = getDisplayId();

        // Get screen reference
        lv_obj_t* screen = getScreen();

        // Set black background
        lv_obj_set_style_bg_color(screen, lv_color_hex(0x000000), 0);

        // Try to load image from MediaCache
        // Check for "image" type (could be PNG or JPEG)
        String cacheId = "d" + String(displayId) + "_image";
        Doki::MediaType imageType;

        _imageData = Doki::MediaCache::getMedia(cacheId, &_imageSize, &imageType);

        if (_imageData == nullptr || _imageSize == 0) {
            showPlaceholder("No image uploaded\n\nUpload via PWA");
            log("No image found for this display");
            return;
        }

        Serial.printf("[ImagePreview] Loading image from cache: %s (%zu KB, type=%d)\n",
                     cacheId.c_str(), _imageSize / 1024, (int)imageType);

        // Validate image type
        if (imageType != Doki::MediaType::IMAGE_PNG &&
            imageType != Doki::MediaType::IMAGE_JPEG) {
            showPlaceholder("Invalid image format");
            log("Error: Invalid image type");
            _imageData = nullptr;
            return;
        }

        // Create LVGL image object
        _image = lv_img_create(screen);
        if (_image == nullptr) {
            log("Error: Failed to create image object");
            showPlaceholder("Error creating image");
            return;
        }

        // Create LVGL image descriptor from memory buffer (use member variable so it persists)
        _imgDsc.header.always_zero = 0;
        _imgDsc.header.w = 0;  // Will be determined by LVGL decoder
        _imgDsc.header.h = 0;
        _imgDsc.data_size = _imageSize;
        _imgDsc.data = _imageData;

        // Set color format based on image type
        if (imageType == Doki::MediaType::IMAGE_PNG) {
            _imgDsc.header.cf = LV_IMG_CF_RAW_ALPHA;  // PNG with alpha
        } else {
            _imgDsc.header.cf = LV_IMG_CF_RAW;  // JPEG without alpha
        }

        // Set image source from memory
        lv_img_set_src(_image, &_imgDsc);

        // Check if image loaded successfully
        const void* src = lv_img_get_src(_image);
        if (src == nullptr) {
            log("Error: Failed to decode image from memory");
            showPlaceholder("Error decoding image");
            lv_obj_del(_image);
            _image = nullptr;
            return;
        }

        // Center the image
        lv_obj_center(_image);

        // Enable anti-aliasing for better quality
        lv_img_set_antialias(_image, true);

        log("✓ Image loaded successfully from PSRAM!");
        Serial.printf("[ImagePreview] Image size: %zu bytes\n", _imageSize);
    }

    void onStart() override {
        log("Image Preview started!");
    }

    void onUpdate() override {
        // Static image - no updates needed
    }

    void onPause() override {
        log("Image Preview paused");
    }

    void onDestroy() override {
        log("Image Preview destroyed");

        // LVGL auto-cleans image resources
        _image = nullptr;
        _placeholderLabel = nullptr;

        // Don't free _imageData - it's managed by MediaCache
        _imageData = nullptr;
        _imageSize = 0;
    }

private:
    lv_obj_t* _image;                ///< LVGL image object
    lv_obj_t* _placeholderLabel;     ///< Placeholder text when no image
    uint8_t* _imageData;             ///< Image data pointer (from MediaCache, don't free!)
    size_t _imageSize;               ///< Image data size
    lv_img_dsc_t _imgDsc;            ///< LVGL image descriptor (must persist)

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
};

#endif // IMAGE_PREVIEW_H
