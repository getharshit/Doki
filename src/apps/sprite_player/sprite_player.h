/**
 * @file sprite_player.h
 * @brief Sprite Player App for Doki OS
 *
 * Plays uploaded .spr animation files using the Animation Manager.
 * Each display has its own sprite slot.
 */

#ifndef SPRITE_PLAYER_H
#define SPRITE_PLAYER_H

#include "doki/app_base.h"
#include "doki/media_service.h"
#include "doki/animation/animation_manager.h"

class SpritePlayerApp : public Doki::DokiApp {
public:
    /**
     * @brief Constructor
     */
    SpritePlayerApp()
        : DokiApp("sprite_player", "Sprite Player"),
          _animId(-1),
          _placeholderLabel(nullptr) {}

    void onCreate() override {
        log("Creating Sprite Player App...");

        // Get display ID for this app
        uint8_t displayId = getDisplayId();

        // Get screen reference
        lv_obj_t* screen = getScreen();

        // Set black background
        lv_obj_set_style_bg_color(screen, lv_color_hex(0x000000), 0);

        // Check if sprite exists for this display
        Doki::MediaInfo info = Doki::MediaService::getMediaInfo(displayId, Doki::MediaType::SPRITE);

        if (!info.exists) {
            showPlaceholder("No sprite uploaded\n\nUpload via PWA");
            log("No sprite found for this display");
            return;
        }

        String logMsg = "Loading sprite from: " + info.path;
        log(logMsg.c_str());

        // Get AnimationManager instance
        auto& mgr = Doki::Animation::AnimationManager::getInstance();

        // Initialize manager if not already done
        if (!mgr.isInitialized()) {
            mgr.init();
        }

        // Create animation options
        Doki::Animation::AnimationOptions options;
        options.autoPlay = false;  // We'll control playback
        options.loopMode = Doki::Animation::LoopMode::LOOP;  // Loop forever

        // Load animation
        _animId = mgr.loadAnimation(info.path.c_str(), screen, options);

        if (_animId < 0) {
            showPlaceholder("Error loading sprite");
            log("Failed to load sprite");
            return;
        }

        // Center the animation (assuming 240×320 display)
        // Will be auto-centered by animation system

        // Start playing with loop mode
        mgr.playAnimation(_animId, Doki::Animation::LoopMode::LOOP);

        log("✓ Sprite loaded and playing!");
        Serial.printf("[SpritePlayer] Sprite size: %zu bytes\n", info.fileSize);
    }

    void onStart() override {
        log("Sprite Player started!");
    }

    void onUpdate() override {
        // Update animation manager to advance frames
        if (_animId >= 0) {
            auto& mgr = Doki::Animation::AnimationManager::getInstance();
            mgr.updateAll();
        }
    }

    void onPause() override {
        log("Sprite Player paused");

        // Pause animation
        if (_animId >= 0) {
            auto& mgr = Doki::Animation::AnimationManager::getInstance();
            mgr.pauseAnimation(_animId);
        }
    }

    void onDestroy() override {
        log("Sprite Player destroyed");

        // Unload animation and free resources
        if (_animId >= 0) {
            auto& mgr = Doki::Animation::AnimationManager::getInstance();
            mgr.unloadAnimation(_animId);
            _animId = -1;
        }

        _placeholderLabel = nullptr;
    }

private:
    int32_t _animId;                 ///< Animation ID from manager
    lv_obj_t* _placeholderLabel;     ///< Placeholder text when no sprite

    /**
     * @brief Show placeholder text when no sprite is available
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

#endif // SPRITE_PLAYER_H
