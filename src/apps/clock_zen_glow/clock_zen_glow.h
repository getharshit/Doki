/**
 * Clock: Zen Glow
 *
 * Minimal design with dot-style typography
 * Features subtle orange gradient glow at bottom for calm aesthetic
 */

#ifndef CLOCK_ZEN_GLOW_H
#define CLOCK_ZEN_GLOW_H

#include "doki/app_base.h"
#include "doki/lvgl_helpers.h"
#include "doki/time_service.h"
#include "assets/fonts/fonts.h"
#include <lvgl.h>
#include <time.h>

using namespace Doki;

class ClockZenGlowApp : public DokiApp {
private:
    lv_obj_t* _timeLabel;        // Large dot-style time
    lv_obj_t* _ampmLabel;        // AM/PM indicator
    lv_obj_t* _dateLabel;        // Date (TUE 09)
    lv_obj_t* _glowCircle;       // Orange glow element at bottom

    uint32_t _lastUpdate;

public:
    ClockZenGlowApp() : DokiApp("clock_zen_glow", "Clock: Zen Glow") {
        _lastUpdate = 0;
    }

    void onCreate() override {
        // Black background
        setBackgroundColor(getScreen(), Colors::BLACK);

        // Create glow element first (behind other elements)
        createGlowEffect();

        // Create UI elements
        createUIElements();

        // Initial update
        updateDisplay();
    }

    void onStart() override {}

    void onUpdate() override {
        uint32_t now = millis();

        // Update every second
        if (now - _lastUpdate >= 1000) {
            _lastUpdate = now;
            updateDisplay();
        }
    }

    void onPause() override {}

    void onDestroy() override {}

private:
    void createGlowEffect() {
        // Create large semi-transparent orange circle at bottom for glow effect
        _glowCircle = lv_obj_create(getScreen());
        lv_obj_set_size(_glowCircle, 320, 320);  // Bigger glow
        lv_obj_align(_glowCircle, LV_ALIGN_BOTTOM_MID, 0, 120);  // Partially off-screen
        lv_obj_set_style_radius(_glowCircle, LV_RADIUS_CIRCLE, 0);
        lv_obj_set_style_bg_color(_glowCircle, lv_color_hex(0xFF6B00), 0);  // Vibrant orange
        lv_obj_set_style_bg_opa(_glowCircle, LV_OPA_40, 0);  // 16% opacity for stronger glow
        lv_obj_set_style_border_width(_glowCircle, 0, 0);
        lv_obj_clear_flag(_glowCircle, LV_OBJ_FLAG_SCROLLABLE);
    }

    void createUIElements() {
        // Display is 240×320
        // Large dot-style time display (centered) - MUCH BIGGER NOW!
        _timeLabel = createStyledLabel(getScreen(), "10:12", Colors::WHITE,
                                       &ballso_96, LV_ALIGN_CENTER, 0, -50);

        // AM/PM indicator (beside time, colorful)
        _ampmLabel = createStyledLabel(getScreen(), "AM", 0xFFD60A,  // Yellow
                                       &inter_medium_20, LV_ALIGN_CENTER, 110, -25);

        // Date at bottom (TUE 09) - larger and colorful
        _dateLabel = createStyledLabel(getScreen(), "TUE 09", 0xFF9F0A,  // Orange
                                       &inter_medium_20, LV_ALIGN_BOTTOM_MID, 0, -35);
    }

    void updateDisplay() {
        // Get current time
        struct tm timeinfo = Doki::TimeService::getInstance().getLocalTime();
        if (timeinfo.tm_year < 100) {
            return;
        }

        // Update time display (12-hour format)
        char timeBuf[16];
        int hour = timeinfo.tm_hour;
        bool isPM = hour >= 12;

        if (hour > 12) hour -= 12;
        if (hour == 0) hour = 12;

        snprintf(timeBuf, sizeof(timeBuf), "%d:%02d", hour, timeinfo.tm_min);
        lv_label_set_text(_timeLabel, timeBuf);

        // Update AM/PM
        lv_label_set_text(_ampmLabel, isPM ? "PM" : "AM");

        // Update date (TUE 09)
        char dateBuf[16];
        const char* days[] = {"SUN", "MON", "TUE", "WED", "THU", "FRI", "SAT"};
        snprintf(dateBuf, sizeof(dateBuf), "%s %02d", days[timeinfo.tm_wday], timeinfo.tm_mday);
        lv_label_set_text(_dateLabel, dateBuf);
    }
};

#endif // CLOCK_ZEN_GLOW_H
