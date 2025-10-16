/**
 * @file gif_player.h
 * @brief Animated GIF Player App for Doki OS
 *
 * Plays animated GIF files loaded from SPIFFS.
 * Each display has its own GIF slot.
 */

#ifndef GIF_PLAYER_H
#define GIF_PLAYER_H

#include "doki/app_base.h"
#include "doki/media_service.h"

class GifPlayerApp : public Doki::DokiApp {
public:
    /**
     * @brief Constructor
     * @param displayId Display ID (0 or 1) to load GIF for
     */
    GifPlayerApp(uint8_t displayId)
        : DokiApp("gif", "GIF Player"),
          _displayId(displayId),
          _gifImage(nullptr),
          _placeholderLabel(nullptr) {}

    void onCreate() override {
        log("Creating GIF Player App...");

        // Get screen reference
        lv_obj_t* screen = getScreen();

        // Set black background
        lv_obj_set_style_bg_color(screen, lv_color_hex(0x000000), 0);

        // Check if GIF exists for this display
        Doki::MediaInfo info = Doki::MediaService::getMediaInfo(_displayId, Doki::MediaType::GIF);

        if (!info.exists) {
            showPlaceholder("No GIF uploaded\n\nUpload via dashboard");
            log("No GIF found for this display");
            return;
        }

        String logMsg = "Loading GIF from: " + info.path;
        log(logMsg.c_str());

        // Create GIF image object
        _gifImage = lv_gif_create(screen);
        if (_gifImage == nullptr) {
            log("Error: Failed to create GIF object");
            showPlaceholder("Error creating GIF");
            return;
        }

        // Set GIF source from SPIFFS path
        // LVGL requires "S:" prefix for SPIFFS driver
        String lvglPath = "S:" + info.path;
        lv_gif_set_src(_gifImage, lvglPath.c_str());

        // Check if GIF loaded successfully
        const void* src = lv_img_get_src(_gifImage);
        if (src == nullptr) {
            log("Error: Failed to load GIF from SPIFFS");
            showPlaceholder("Error loading GIF");
            lv_obj_del(_gifImage);
            _gifImage = nullptr;
            return;
        }

        // Center the GIF
        lv_obj_center(_gifImage);

        // GIF animation starts automatically

        log("✓ GIF loaded and playing!");
        Serial.printf("[GifPlayer] GIF size: %zu bytes\n", info.fileSize);
    }

    void onStart() override {
        log("GIF Player started!");
    }

    void onUpdate() override {
        // LVGL handles GIF animation automatically
        // No manual frame management needed
    }

    void onPause() override {
        log("GIF Player paused");
        // Optional: Pause GIF animation
    }

    void onDestroy() override {
        log("GIF Player destroyed");
        // LVGL auto-cleans GIF resources
        _gifImage = nullptr;
        _placeholderLabel = nullptr;
    }

private:
    uint8_t _displayId;              ///< Display ID (0 or 1)
    lv_obj_t* _gifImage;             ///< LVGL GIF object
    lv_obj_t* _placeholderLabel;     ///< Placeholder text when no GIF

    /**
     * @brief Show placeholder text when no GIF is available
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

#endif // GIF_PLAYER_H
