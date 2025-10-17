/**
 * @file clock_app.h
 * @brief FULLY NON-BLOCKING Clock App - NTP runs in background task
 * 
 * CRITICAL FIX: NTPClient library blocks the main loop!
 * Solution: Run NTP updates in a separate FreeRTOS task
 */

#ifndef CLOCK_APP_H
#define CLOCK_APP_H

#include "doki/app_base.h"
#include "doki/lvgl_helpers.h"
#include "timing_constants.h"
#include <NTPClient.h>
#include <WiFiUdp.h>

using namespace Doki;

class ClockApp : public Doki::DokiApp {
public:
    ClockApp() : DokiApp("clock", "Clock") {
        _ntpClient = nullptr;
        _udp = nullptr;
        _ntpTaskHandle = nullptr;
        _timeValid = false;
    }
    
    void onCreate() override {
        log("Creating NON-BLOCKING Clock App...");
        
        // Initialize NTP objects (but don't start yet)
        _udp = new WiFiUDP();
        _ntpClient = new NTPClient(*_udp, "pool.ntp.org", 19800, UPDATE_INTERVAL_NTP_RETRY_MS);
        
        // Static background circles
        _bgCircle1 = lv_obj_create(getScreen());
        lv_obj_set_size(_bgCircle1, 180, 180);
        lv_obj_align(_bgCircle1, LV_ALIGN_CENTER, -30, -40);
        lv_obj_set_style_radius(_bgCircle1, LV_RADIUS_CIRCLE, 0);
        lv_obj_set_style_bg_color(_bgCircle1, lv_color_hex(0x667eea), 0);
        lv_obj_set_style_bg_opa(_bgCircle1, 38, 0);
        lv_obj_set_style_border_width(_bgCircle1, 0, 0);
        
        _bgCircle2 = lv_obj_create(getScreen());
        lv_obj_set_size(_bgCircle2, 220, 220);
        lv_obj_align(_bgCircle2, LV_ALIGN_CENTER, 20, -30);
        lv_obj_set_style_radius(_bgCircle2, LV_RADIUS_CIRCLE, 0);
        lv_obj_set_style_bg_color(_bgCircle2, lv_color_hex(0x764ba2), 0);
        lv_obj_set_style_bg_opa(_bgCircle2, 38, 0);
        lv_obj_set_style_border_width(_bgCircle2, 0, 0);
        
        // Time label (large, centered)
        _timeLabel = createValueLabel(getScreen(), "Syncing...", 0x667eea, -50);

        // AM/PM label (small, next to time)
        _ampmLabel = createStyledLabel(getScreen(), "", Colors::GRAY,
                                       &lv_font_montserrat_18, LV_ALIGN_CENTER, 85, -60);

        // Date label
        _dateLabel = createStyledLabel(getScreen(), "Please wait...", Colors::DARK_GRAY,
                                       &lv_font_montserrat_16, LV_ALIGN_CENTER, 0, 0);

        // Day label
        _dayLabel = createInfoLabel(getScreen(), "", LV_ALIGN_CENTER, 0, 25);
        
        // Day progress bar
        _dayProgress = createProgressBar(getScreen(), 200, 8, Colors::LIGHT_GRAY, 0x667eea,
                                        LV_ALIGN_CENTER, 0, 50);
        lv_bar_set_range(_dayProgress, 0, 86400);
        lv_obj_set_style_radius(_dayProgress, 4, 0);

        // Progress percentage
        _progressLabel = createInfoLabel(getScreen(), "", LV_ALIGN_CENTER, 0, 65);

        // Seconds bar
        _secondsBar = createProgressBar(getScreen(), 200, 4, Colors::LIGHT_GRAY, Colors::SUCCESS,
                                       LV_ALIGN_CENTER, 0, -20);
        lv_bar_set_range(_secondsBar, 0, 60);
        lv_obj_set_style_radius(_secondsBar, 2, 0);
        lv_obj_set_style_border_width(_secondsBar, 0, 0);
        
        // Uptime
        _uptimeLabel = createInfoLabel(getScreen(), "", LV_ALIGN_BOTTOM_MID, 0, -10);
        
        _lastUpdate = 0;
        _timeValid = false;
    }
    
    void onStart() override {
        log("Clock App started!");
        
        // Start NTP client in BACKGROUND TASK (non-blocking!)
        _ntpClient->begin();
        
        // Create FreeRTOS task for NTP updates
        xTaskCreatePinnedToCore(
            ntpUpdateTask,        // Task function
            "NTP_Update",         // Task name
            4096,                 // Stack size
            this,                 // Parameter (pass 'this' pointer)
            1,                    // Priority (low)
            &_ntpTaskHandle,      // Task handle
            0                     // Core 0
        );
        
        log("NTP background task started");
    }
    
    void onUpdate() override {
        uint32_t now = millis();
        
        // Update display every 1 second
        if (now - _lastUpdate >= 1000) {
            updateDisplay();
            _lastUpdate = now;
        }
    }
    
    void onPause() override {
        log("Clock App paused");
    }
    
    void onDestroy() override {
        log("Clock App destroyed");
        
        // Stop NTP background task
        if (_ntpTaskHandle != nullptr) {
            vTaskDelete(_ntpTaskHandle);
            _ntpTaskHandle = nullptr;
            log("NTP task stopped");
        }
        
        if (_ntpClient) {
            _ntpClient->end();
            delete _ntpClient;
            _ntpClient = nullptr;
        }
        
        if (_udp) {
            delete _udp;
            _udp = nullptr;
        }
    }

private:
    lv_obj_t* _bgCircle1;
    lv_obj_t* _bgCircle2;
    lv_obj_t* _timeLabel;
    lv_obj_t* _ampmLabel;
    lv_obj_t* _dateLabel;
    lv_obj_t* _dayLabel;
    lv_obj_t* _dayProgress;
    lv_obj_t* _progressLabel;
    lv_obj_t* _secondsBar;
    lv_obj_t* _uptimeLabel;
    
    NTPClient* _ntpClient;
    WiFiUDP* _udp;
    TaskHandle_t _ntpTaskHandle;
    
    uint32_t _lastUpdate;
    volatile bool _timeValid;  // Volatile because accessed from multiple tasks
    
    // Static task function for FreeRTOS
    static void ntpUpdateTask(void* parameter) {
        ClockApp* self = static_cast<ClockApp*>(parameter);
        
        Serial.println("[ClockApp NTP Task] Starting...");
        
        // Initial sync (this WILL block, but only in background task!)
        self->_ntpClient->forceUpdate();
        self->_timeValid = true;
        Serial.println("[ClockApp NTP Task] Initial sync complete");
        
        // Update every 60 seconds
        while (true) {
            vTaskDelay(pdMS_TO_TICKS(60000));  // Wait 60 seconds
            
            // Try to update (this may block, but only blocks THIS task)
            if (self->_ntpClient->update()) {
                Serial.println("[ClockApp NTP Task] Time updated");
            }
        }
    }
    
    void updateDisplay() {
        if (!_ntpClient || !_timeValid) {
            // Still syncing
            return;
        }
        
        time_t epochTime = _ntpClient->getEpochTime();
        if (epochTime < 1000000000) {
            // Not synced yet
            return;
        }
        
        struct tm* timeInfo = localtime(&epochTime);
        
        // Format time
        int hour = timeInfo->tm_hour;
        const char* ampm = (hour >= 12) ? "PM" : "AM";
        if (hour > 12) hour -= 12;
        if (hour == 0) hour = 12;
        
        char timeBuf[16];
        snprintf(timeBuf, sizeof(timeBuf), "%02d:%02d:%02d", 
                 hour, timeInfo->tm_min, timeInfo->tm_sec);
        lv_label_set_text(_timeLabel, timeBuf);
        lv_label_set_text(_ampmLabel, ampm);
        
        // Update seconds bar
        lv_bar_set_value(_secondsBar, timeInfo->tm_sec, LV_ANIM_OFF);
        
        // Date
        const char* months[] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun",
                               "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};
        char dateBuf[32];
        snprintf(dateBuf, sizeof(dateBuf), "%02d %s %d",
                 timeInfo->tm_mday, months[timeInfo->tm_mon], 
                 timeInfo->tm_year + 1900);
        lv_label_set_text(_dateLabel, dateBuf);
        
        // Day
        const char* days[] = {"Sunday", "Monday", "Tuesday", "Wednesday",
                             "Thursday", "Friday", "Saturday"};
        lv_label_set_text(_dayLabel, days[timeInfo->tm_wday]);
        
        // Day progress
        int secondsSinceMidnight = timeInfo->tm_hour * 3600 + 
                                   timeInfo->tm_min * 60 + 
                                   timeInfo->tm_sec;
        lv_bar_set_value(_dayProgress, secondsSinceMidnight, LV_ANIM_OFF);
        
        float dayPercent = (float)secondsSinceMidnight / 864.0f;
        char progressBuf[16];
        snprintf(progressBuf, sizeof(progressBuf), "Day: %.0f%%", dayPercent);
        lv_label_set_text(_progressLabel, progressBuf);
        
        // Uptime
        uint32_t uptime = getUptime() / 1000;
        char uptimeBuf[32];
        if (uptime < 60) {
            snprintf(uptimeBuf, sizeof(uptimeBuf), "Uptime: %lus", uptime);
        } else if (uptime < 3600) {
            snprintf(uptimeBuf, sizeof(uptimeBuf), "Uptime: %lum %lus", 
                     uptime / 60, uptime % 60);
        } else {
            snprintf(uptimeBuf, sizeof(uptimeBuf), "Uptime: %luh %lum", 
                     uptime / 3600, (uptime % 3600) / 60);
        }
        lv_label_set_text(_uptimeLabel, uptimeBuf);
    }
};

#endif // CLOCK_APP_H