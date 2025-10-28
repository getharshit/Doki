/**
 * Clock: Neon Blocks
 *
 * Yellow & Black boxy industrial design
 * Sharp-edged blocks with yellow accents on pure black
 */

#ifndef CLOCK_NEON_BLOCKS_H
#define CLOCK_NEON_BLOCKS_H

#include "doki/app_base.h"
#include "doki/lvgl_helpers.h"
#include "doki/time_service.h"
#include "assets/fonts/fonts.h"
#include <lvgl.h>
#include <time.h>

using namespace Doki;

class ClockNeonBlocksApp : public DokiApp {
private:
    // UI Elements - Boxy blocks
    lv_obj_t* _timeContainer;      // Time block
    lv_obj_t* _timeLabel;          // Large time display
    lv_obj_t* _ampmContainer;      // AM/PM block
    lv_obj_t* _ampmLabel;          // AM/PM text

    lv_obj_t* _dateContainer;      // Date block
    lv_obj_t* _dateLabel;          // Date text

    // Three stat blocks
    lv_obj_t* _dayContainer;       // Day stat block
    lv_obj_t* _dayLabel;
    lv_obj_t* _dayValue;

    lv_obj_t* _tempContainer;      // Temp stat block
    lv_obj_t* _tempLabel;
    lv_obj_t* _tempValue;

    lv_obj_t* _humidityContainer;  // Humidity stat block
    lv_obj_t* _humidityLabel;
    lv_obj_t* _humidityValue;

    // Update tracking
    uint32_t _lastUpdate;

public:
    ClockNeonBlocksApp() : DokiApp("clock_neon_blocks", "Clock: Neon Blocks") {
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
        // YELLOW & BLACK BOXY THEME

        const uint32_t YELLOW = 0xFFD700;   // Gold yellow
        const uint32_t BLACK = 0x000000;     // Pure black
        const uint32_t ORANGE = 0xFFA500;    // Orange accent

        // ==========================================
        // TOP: TIME BLOCKS (0-110px)
        // ==========================================

        // Large time block
        _timeContainer = lv_obj_create(getScreen());
        lv_obj_set_size(_timeContainer, 160, 85);
        lv_obj_set_pos(_timeContainer, 15, 20);
        lv_obj_set_style_bg_color(_timeContainer, lv_color_hex(BLACK), 0);
        lv_obj_set_style_border_color(_timeContainer, lv_color_hex(YELLOW), 0);
        lv_obj_set_style_border_width(_timeContainer, 4, 0);
        lv_obj_set_style_radius(_timeContainer, 2, 0);  // Very boxy
        lv_obj_set_style_pad_all(_timeContainer, 0, 0);

        _timeLabel = createStyledLabel(_timeContainer, "12:00", YELLOW,
                                       &ka1_80, LV_ALIGN_CENTER, 0, 0);

        // AM/PM block
        _ampmContainer = lv_obj_create(getScreen());
        lv_obj_set_size(_ampmContainer, 45, 85);
        lv_obj_set_pos(_ampmContainer, 180, 20);
        lv_obj_set_style_bg_color(_ampmContainer, lv_color_hex(YELLOW), 0);
        lv_obj_set_style_border_width(_ampmContainer, 0, 0);
        lv_obj_set_style_radius(_ampmContainer, 2, 0);
        lv_obj_set_style_pad_all(_ampmContainer, 0, 0);

        _ampmLabel = createStyledLabel(_ampmContainer, "PM", BLACK,
                                       &ka1_36, LV_ALIGN_CENTER, 0, 0);

        // ==========================================
        // MIDDLE: DATE BLOCK (115-160px)
        // ==========================================

        _dateContainer = lv_obj_create(getScreen());
        lv_obj_set_size(_dateContainer, 210, 38);
        lv_obj_set_pos(_dateContainer, 15, 115);
        lv_obj_set_style_bg_color(_dateContainer, lv_color_hex(BLACK), 0);
        lv_obj_set_style_border_color(_dateContainer, lv_color_hex(ORANGE), 0);
        lv_obj_set_style_border_width(_dateContainer, 3, 0);
        lv_obj_set_style_radius(_dateContainer, 2, 0);
        lv_obj_set_style_pad_all(_dateContainer, 0, 0);

        _dateLabel = createStyledLabel(_dateContainer, "MON JAN 15", ORANGE,
                                       &ka1_20, LV_ALIGN_CENTER, 0, 0);

        // ==========================================
        // BOTTOM: THREE STAT BLOCKS (165-320px)
        // ==========================================

        int blockWidth = 62;
        int blockHeight = 110;
        int blockY = 175;
        int spacing = 12;
        int startX = 15;

        // --- DAY BLOCK ---
        _dayContainer = lv_obj_create(getScreen());
        lv_obj_set_size(_dayContainer, blockWidth, blockHeight);
        lv_obj_set_pos(_dayContainer, startX, blockY);
        lv_obj_set_style_bg_color(_dayContainer, lv_color_hex(BLACK), 0);
        lv_obj_set_style_border_color(_dayContainer, lv_color_hex(YELLOW), 0);
        lv_obj_set_style_border_width(_dayContainer, 3, 0);
        lv_obj_set_style_radius(_dayContainer, 2, 0);
        lv_obj_set_style_pad_all(_dayContainer, 0, 0);

        _dayLabel = createStyledLabel(_dayContainer, "DAY", YELLOW,
                                     &ka1_20, LV_ALIGN_TOP_MID, 0, 10);
        _dayValue = createStyledLabel(_dayContainer, "0", YELLOW,
                                      &ka1_48, LV_ALIGN_CENTER, 0, 10);

        // --- TEMP BLOCK ---
        int tempX = startX + blockWidth + spacing;
        _tempContainer = lv_obj_create(getScreen());
        lv_obj_set_size(_tempContainer, blockWidth, blockHeight);
        lv_obj_set_pos(_tempContainer, tempX, blockY);
        lv_obj_set_style_bg_color(_tempContainer, lv_color_hex(BLACK), 0);
        lv_obj_set_style_border_color(_tempContainer, lv_color_hex(YELLOW), 0);
        lv_obj_set_style_border_width(_tempContainer, 3, 0);
        lv_obj_set_style_radius(_tempContainer, 2, 0);
        lv_obj_set_style_pad_all(_tempContainer, 0, 0);

        _tempLabel = createStyledLabel(_tempContainer, "TMP", YELLOW,
                                       &ka1_20, LV_ALIGN_TOP_MID, 0, 10);
        _tempValue = createStyledLabel(_tempContainer, "28C", YELLOW,
                                       &ka1_36, LV_ALIGN_CENTER, 0, 10);

        // --- HUMIDITY BLOCK ---
        int humX = tempX + blockWidth + spacing;
        _humidityContainer = lv_obj_create(getScreen());
        lv_obj_set_size(_humidityContainer, blockWidth, blockHeight);
        lv_obj_set_pos(_humidityContainer, humX, blockY);
        lv_obj_set_style_bg_color(_humidityContainer, lv_color_hex(BLACK), 0);
        lv_obj_set_style_border_color(_humidityContainer, lv_color_hex(YELLOW), 0);
        lv_obj_set_style_border_width(_humidityContainer, 3, 0);
        lv_obj_set_style_radius(_humidityContainer, 2, 0);
        lv_obj_set_style_pad_all(_humidityContainer, 0, 0);

        _humidityLabel = createStyledLabel(_humidityContainer, "HUM", YELLOW,
                                           &ka1_20, LV_ALIGN_TOP_MID, 0, 10);
        _humidityValue = createStyledLabel(_humidityContainer, "45", YELLOW,
                                           &ka1_36, LV_ALIGN_CENTER, 0, 10);
    }

    void updateDisplay() {
        // Get current time
        struct tm timeinfo = Doki::TimeService::getInstance().getLocalTime();
        if (timeinfo.tm_year < 100) {
            return;
        }

        // Update time display
        char timeBuf[16];
        int hour = timeinfo.tm_hour;
        bool isPM = hour >= 12;

        // Convert to 12-hour format
        if (hour > 12) hour -= 12;
        if (hour == 0) hour = 12;

        snprintf(timeBuf, sizeof(timeBuf), "%d:%02d", hour, timeinfo.tm_min);
        lv_label_set_text(_timeLabel, timeBuf);

        // Update AM/PM
        lv_label_set_text(_ampmLabel, isPM ? "PM" : "AM");

        // Update date
        const char* days[] = {"SUN", "MON", "TUE", "WED", "THU", "FRI", "SAT"};
        const char* months[] = {"JAN", "FEB", "MAR", "APR", "MAY", "JUN",
                                "JUL", "AUG", "SEP", "OCT", "NOV", "DEC"};
        char dateBuf[32];
        snprintf(dateBuf, sizeof(dateBuf), "%s %s %d",
                days[timeinfo.tm_wday], months[timeinfo.tm_mon], timeinfo.tm_mday);
        lv_label_set_text(_dateLabel, dateBuf);

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

#endif // CLOCK_NEON_BLOCKS_H
