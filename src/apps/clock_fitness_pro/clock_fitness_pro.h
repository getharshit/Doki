/**
 * Clock: Fitness Pro
 *
 * Modern vertical split design
 * Hours on top, minutes on bottom with different colors
 * Stats on left and right sides
 */

#ifndef CLOCK_FITNESS_PRO_H
#define CLOCK_FITNESS_PRO_H

#include "doki/app_base.h"
#include "doki/lvgl_helpers.h"
#include "doki/time_service.h"
#include "assets/fonts/fonts.h"
#include <lvgl.h>
#include <time.h>

using namespace Doki;

class ClockFitnessProApp : public DokiApp {
private:
    // Vertical time split
    lv_obj_t* _hoursLabel;        // Hours (top)
    lv_obj_t* _minutesLabel;      // Minutes (bottom)

    // Side stats
    lv_obj_t* _dayLabel;          // Left top
    lv_obj_t* _dayValue;
    lv_obj_t* _tempLabel;         // Right top
    lv_obj_t* _tempValue;
    lv_obj_t* _humidityLabel;     // Left bottom
    lv_obj_t* _humidityValue;
    lv_obj_t* _ampmLabel;         // Right bottom

    uint32_t _lastUpdate;

public:
    ClockFitnessProApp() : DokiApp("clock_fitness_pro", "Clock: Fitness Pro") {
        _lastUpdate = 0;
    }

    void onCreate() override {
        // Black background
        setBackgroundColor(getScreen(), Colors::BLACK);

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
    void createUIElements() {
        // Display is 240×320
        // MODERN VERTICAL SPLIT DESIGN

        const uint32_t CYAN = 0x00D9FF;      // Hours color
        const uint32_t ORANGE = 0xFF9F0A;    // Minutes color
        const uint32_t GREEN = 0x30D158;     // Accent color
        const uint32_t GRAY = 0x808080;      // Secondary text

        // ==========================================
        // TOP: HOURS (0-140px)
        // ==========================================

        _hoursLabel = createStyledLabel(getScreen(), "10", CYAN,
                                       &inter_black_96, LV_ALIGN_TOP_MID, 0, 25);

        // ==========================================
        // MIDDLE: SIDE STATS - TOP ROW (90-140px)
        // ==========================================

        // Left: DAY
        _dayLabel = createStyledLabel(getScreen(), "DAY", GREEN,
                                     &inter_black_16, LV_ALIGN_TOP_LEFT, 20, 95);
        _dayValue = createStyledLabel(getScreen(), "0", GREEN,
                                      &inter_semibold_28, LV_ALIGN_TOP_LEFT, 20, 115);

        // Right: TEMP
        _tempLabel = createStyledLabel(getScreen(), "TEMP", ORANGE,
                                       &inter_black_16, LV_ALIGN_TOP_RIGHT, -20, 95);
        _tempValue = createStyledLabel(getScreen(), "28C", ORANGE,
                                       &inter_semibold_28, LV_ALIGN_TOP_RIGHT, -20, 115);

        // ==========================================
        // CENTER: MINUTES (180-280px)
        // ==========================================

        _minutesLabel = createStyledLabel(getScreen(), "28", ORANGE,
                                         &inter_black_96, LV_ALIGN_TOP_MID, 0, 180);

        // ==========================================
        // BOTTOM: SIDE STATS - BOTTOM ROW (245-295px)
        // ==========================================

        // Left: HUMIDITY
        _humidityLabel = createStyledLabel(getScreen(), "HUM", CYAN,
                                           &inter_black_16, LV_ALIGN_TOP_LEFT, 20, 250);
        _humidityValue = createStyledLabel(getScreen(), "45", CYAN,
                                           &inter_semibold_28, LV_ALIGN_TOP_LEFT, 20, 270);

        // Right: AM/PM
        _ampmLabel = createStyledLabel(getScreen(), "AM", GRAY,
                                       &inter_semibold_28, LV_ALIGN_TOP_RIGHT, -20, 265);
    }

    void updateDisplay() {
        // Get current time
        struct tm timeinfo = Doki::TimeService::getInstance().getLocalTime();
        if (timeinfo.tm_year < 100) {
            return;
        }

        // Update hours and minutes separately (12-hour format)
        int hour = timeinfo.tm_hour;
        bool isPM = hour >= 12;

        if (hour > 12) hour -= 12;
        if (hour == 0) hour = 12;

        // Update hours
        char hourBuf[8];
        snprintf(hourBuf, sizeof(hourBuf), "%d", hour);
        lv_label_set_text(_hoursLabel, hourBuf);

        // Update minutes
        char minBuf[8];
        snprintf(minBuf, sizeof(minBuf), "%02d", timeinfo.tm_min);
        lv_label_set_text(_minutesLabel, minBuf);

        // Update AM/PM
        lv_label_set_text(_ampmLabel, isPM ? "PM" : "AM");

        // Update day progress
        uint32_t secondsSinceMidnight = (timeinfo.tm_hour * 3600) +
                                        (timeinfo.tm_min * 60) +
                                        timeinfo.tm_sec;
        uint32_t dayProgress = (secondsSinceMidnight * 100) / 86400;

        char dayBuf[8];
        snprintf(dayBuf, sizeof(dayBuf), "%lu", dayProgress);
        lv_label_set_text(_dayValue, dayBuf);

        // Update temperature (mock data)
        int temp = 28;  // Mock temperature
        char tempBuf[8];
        snprintf(tempBuf, sizeof(tempBuf), "%dC", temp);
        lv_label_set_text(_tempValue, tempBuf);

        // Update humidity (mock data)
        int humidity = 45;  // Mock humidity
        char humBuf[8];
        snprintf(humBuf, sizeof(humBuf), "%d", humidity);
        lv_label_set_text(_humidityValue, humBuf);
    }
};

#endif // CLOCK_FITNESS_PRO_H
