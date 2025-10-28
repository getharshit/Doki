/**
 * Clock: Rainbow Wave
 *
 * Modern Apple Watch style with gradient mesh background
 * Clean, bold typography with shadow effects for contrast
 */

#ifndef CLOCK_RAINBOW_WAVE_H
#define CLOCK_RAINBOW_WAVE_H

#include "doki/app_base.h"
#include "doki/lvgl_helpers.h"
#include "doki/time_service.h"
#include "assets/fonts/fonts.h"
#include <lvgl.h>
#include <time.h>

using namespace Doki;

class ClockRainbowWaveApp : public DokiApp {
private:
    lv_obj_t* _timeLabel;        // Giant time display
    lv_obj_t* _dateLabel;        // Date text
    lv_obj_t* _weatherLabel;     // Weather temp
    lv_obj_t* _weatherIcon;      // Weather icon placeholder

    uint32_t _lastUpdate;

public:
    ClockRainbowWaveApp() : DokiApp("clock_rainbow_wave", "Clock: Rainbow Wave") {
        _lastUpdate = 0;
    }

    void onCreate() override {
        // Create vibrant gradient background (deep blue to purple)
        setGradientBackground(getScreen(), 0x0A1F44, 0x6B21A8);

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
        // Giant time display (centered, moved up)
        _timeLabel = createStyledLabel(getScreen(), "10:09", Colors::WHITE,
                                       &inter_bold_84, LV_ALIGN_CENTER, 0, -40);
        addTextShadow(_timeLabel, 15, 4);  // Stronger shadow

        // Date text below time (with better spacing)
        _dateLabel = createStyledLabel(getScreen(), "MONDAY, JAN 15", 0xE0E0E0,  // Light gray
                                       &inter_medium_20, LV_ALIGN_CENTER, 0, 55);
        addTextShadow(_dateLabel, 10, 3);

        // Weather section (top right, larger)
        _weatherIcon = createIconPlaceholder(getScreen(), 28, 0xFFD60A,  // Yellow
                                            LV_ALIGN_TOP_RIGHT, -50, 12);
        addTextShadow(_weatherIcon, 8, 2);

        _weatherLabel = createStyledLabel(getScreen(), "22°", Colors::WHITE,
                                          &inter_medium_18, LV_ALIGN_TOP_RIGHT, -12, 15);
        addTextShadow(_weatherLabel, 10, 3);
    }

    void updateDisplay() {
        // Get current time
        struct tm timeinfo = Doki::TimeService::getInstance().getLocalTime();
        if (timeinfo.tm_year < 100) {
            return;
        }

        // Update time display (24-hour format)
        char timeBuf[16];
        snprintf(timeBuf, sizeof(timeBuf), "%02d:%02d", timeinfo.tm_hour, timeinfo.tm_min);
        lv_label_set_text(_timeLabel, timeBuf);

        // Update date text (uppercase for modern look)
        char dateBuf[64];
        const char* days[] = {"SUNDAY", "MONDAY", "TUESDAY", "WEDNESDAY",
                              "THURSDAY", "FRIDAY", "SATURDAY"};
        const char* months[] = {"JAN", "FEB", "MAR", "APR", "MAY", "JUN",
                                "JUL", "AUG", "SEP", "OCT", "NOV", "DEC"};
        snprintf(dateBuf, sizeof(dateBuf), "%s, %s %d",
                days[timeinfo.tm_wday], months[timeinfo.tm_mon], timeinfo.tm_mday);
        lv_label_set_text(_dateLabel, dateBuf);

        // Weather temp (mock)
        char weatherBuf[16];
        int temp = 22;  // Mock temperature
        snprintf(weatherBuf, sizeof(weatherBuf), "%d°", temp);
        lv_label_set_text(_weatherLabel, weatherBuf);
    }
};

#endif // CLOCK_RAINBOW_WAVE_H
