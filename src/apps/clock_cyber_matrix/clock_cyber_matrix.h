/**
 * Clock: Cyber Matrix
 *
 * Retro dot matrix LED style display
 * Monochrome white-on-black design with pixelated typography
 */

#ifndef CLOCK_CYBER_MATRIX_H
#define CLOCK_CYBER_MATRIX_H

#include "doki/app_base.h"
#include "doki/lvgl_helpers.h"
#include "doki/time_service.h"
#include "assets/fonts/fonts.h"
#include <lvgl.h>
#include <time.h>

using namespace Doki;

class ClockCyberMatrixApp : public DokiApp {
private:
    lv_obj_t* _dateTextLabel;      // Full date (MON, JUL 28)
    lv_obj_t* _timeLabel;          // Large time (10:09)
    lv_obj_t* _calendarIcon;       // Calendar icon placeholder
    lv_obj_t* _calendarDateLabel;  // Calendar date number
    lv_obj_t* _secondTimeLabel;    // Secondary time (11:00AM)
    lv_obj_t* _stepsLabel;         // Steps counter
    lv_obj_t* _batteryLabel;       // Battery percentage

    uint32_t _lastUpdate;
    int _mockSteps;

public:
    ClockCyberMatrixApp() : DokiApp("clock_cyber_matrix", "Clock: Cyber Matrix") {
        _lastUpdate = 0;
        _mockSteps = 2444;
    }

    void onCreate() override {
        // Black background
        setBackgroundColor(getScreen(), Colors::BLACK);

        // Create UI elements with dot matrix style
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

            // Simulate step increments
            if (rand() % 30 == 0) {
                _mockSteps += rand() % 5;
            }
        }
    }

    void onPause() override {}

    void onDestroy() override {}

private:
    void createUIElements() {
        // Display is 240×320 - use full vertical space
        // Full date at top (MON, JUL 28)
        _dateTextLabel = createStyledLabel(getScreen(), "MON, JUL 28", 0x00FF41,  // Bright green
                                           &pixel_operator_18, LV_ALIGN_TOP_MID, 0, 10);

        // Large time display (centered) - BIGGER NOW!
        _timeLabel = createStyledLabel(getScreen(), "10:09", 0x00FF41,  // Bright green
                                       &pixel_operator_72, LV_ALIGN_CENTER, 0, -20);

        // Calendar icon (bright cyan square)
        _calendarIcon = createIconPlaceholder(getScreen(), 18, 0x00D9FF,  // Cyan
                                              LV_ALIGN_TOP_LEFT, 10, 45);

        // Calendar date number beside icon
        _calendarDateLabel = createStyledLabel(getScreen(), "28", 0x00FF41,
                                               &pixel_operator_14, LV_ALIGN_TOP_LEFT, 35, 47);

        // Secondary time (12-hour format below main time)
        _secondTimeLabel = createStyledLabel(getScreen(), "11:00AM", 0xFFD60A,  // Yellow
                                             &pixel_operator_18, LV_ALIGN_CENTER, 0, 70);

        // Steps counter (with icon placeholder)
        lv_obj_t* stepsIcon = createIconPlaceholder(getScreen(), 18, 0xFF10F0,  // Magenta
                                                    LV_ALIGN_BOTTOM_LEFT, 10, -45);
        _stepsLabel = createStyledLabel(getScreen(), "2444", 0x00FF41,
                                        &pixel_operator_14, LV_ALIGN_BOTTOM_LEFT, 35, -43);

        // Battery percentage
        lv_obj_t* batteryIcon = createIconPlaceholder(getScreen(), 18, 0x30D158,  // Green
                                                      LV_ALIGN_TOP_RIGHT, -65, 45);
        _batteryLabel = createStyledLabel(getScreen(), "86%", 0x00FF41,
                                          &pixel_operator_14, LV_ALIGN_TOP_RIGHT, -10, 47);
    }

    void updateDisplay() {
        // Get current time
        struct tm timeinfo = Doki::TimeService::getInstance().getLocalTime();
        if (timeinfo.tm_year < 100) {
            return;
        }

        // Update date text (MON, JUL 28)
        char dateBuf[32];
        const char* days[] = {"SUN", "MON", "TUE", "WED", "THU", "FRI", "SAT"};
        const char* months[] = {"JAN", "FEB", "MAR", "APR", "MAY", "JUN",
                                "JUL", "AUG", "SEP", "OCT", "NOV", "DEC"};
        snprintf(dateBuf, sizeof(dateBuf), "%s, %s %d",
                days[timeinfo.tm_wday], months[timeinfo.tm_mon], timeinfo.tm_mday);
        lv_label_set_text(_dateTextLabel, dateBuf);

        // Update large time (24-hour format for retro look)
        char timeBuf[16];
        snprintf(timeBuf, sizeof(timeBuf), "%02d:%02d", timeinfo.tm_hour, timeinfo.tm_min);
        lv_label_set_text(_timeLabel, timeBuf);

        // Update calendar date
        char dayBuf[8];
        snprintf(dayBuf, sizeof(dayBuf), "%d", timeinfo.tm_mday);
        lv_label_set_text(_calendarDateLabel, dayBuf);

        // Update secondary time (12-hour format)
        char secTimeBuf[16];
        int hour = timeinfo.tm_hour;
        const char* ampm = hour >= 12 ? "PM" : "AM";
        if (hour > 12) hour -= 12;
        if (hour == 0) hour = 12;
        snprintf(secTimeBuf, sizeof(secTimeBuf), "%d:%02d%s", hour, timeinfo.tm_min, ampm);
        lv_label_set_text(_secondTimeLabel, secTimeBuf);

        // Update steps
        char stepsBuf[16];
        snprintf(stepsBuf, sizeof(stepsBuf), "%d", _mockSteps);
        lv_label_set_text(_stepsLabel, stepsBuf);

        // Battery percentage (mock - could integrate with real battery if available)
        char batteryBuf[16];
        int batteryPercent = 86;  // Mock value
        snprintf(batteryBuf, sizeof(batteryBuf), "%d%%", batteryPercent);
        lv_label_set_text(_batteryLabel, batteryBuf);
    }
};

#endif // CLOCK_CYBER_MATRIX_H
