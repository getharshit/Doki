/**
 * @file clock_app.h
 * @brief Enhanced Clock App with animations and progress bars
 */

#ifndef CLOCK_APP_H
#define CLOCK_APP_H

#include "doki/app_base.h"
#include <NTPClient.h>
#include <WiFiUdp.h>

class ClockApp : public Doki::DokiApp {
public:
    ClockApp() : DokiApp("clock", "Clock") {
        _ntpClient = nullptr;
        _udp = nullptr;
    }
    
    void onCreate() override {
        log("Creating Enhanced Clock App...");
        
        // Initialize NTP
        _udp = new WiFiUDP();
        _ntpClient = new NTPClient(*_udp, "pool.ntp.org", 19800, 60000);
        _ntpClient->begin();
        
        // Create animated background circles
        _bgCircle1 = lv_obj_create(getScreen());
        lv_obj_set_size(_bgCircle1, 180, 180);
        lv_obj_align(_bgCircle1, LV_ALIGN_CENTER, -30, -40);
        lv_obj_set_style_radius(_bgCircle1, LV_RADIUS_CIRCLE, 0);
        lv_obj_set_style_bg_color(_bgCircle1, lv_color_hex(0x667eea), 0);
        lv_obj_set_style_bg_opa(_bgCircle1, LV_OPA_10, 0);
        lv_obj_set_style_border_width(_bgCircle1, 0, 0);
        
        _bgCircle2 = lv_obj_create(getScreen());
        lv_obj_set_size(_bgCircle2, 220, 220);
        lv_obj_align(_bgCircle2, LV_ALIGN_CENTER, 20, -30);
        lv_obj_set_style_radius(_bgCircle2, LV_RADIUS_CIRCLE, 0);
        lv_obj_set_style_bg_color(_bgCircle2, lv_color_hex(0x764ba2), 0);
        lv_obj_set_style_bg_opa(_bgCircle2, LV_OPA_10, 0);
        lv_obj_set_style_border_width(_bgCircle2, 0, 0);
        
        // Time label with shadow effect
        _timeLabel = lv_label_create(getScreen());
        lv_label_set_text(_timeLabel, "--:--:--");
        lv_obj_align(_timeLabel, LV_ALIGN_CENTER, 0, -50);
        lv_obj_set_style_text_font(_timeLabel, &lv_font_montserrat_48, 0);
        lv_obj_set_style_text_color(_timeLabel, lv_color_hex(0x667eea), 0);
        
        // AM/PM label
        _ampmLabel = lv_label_create(getScreen());
        lv_label_set_text(_ampmLabel, "AM");
        lv_obj_align(_ampmLabel, LV_ALIGN_CENTER, 85, -60);
        lv_obj_set_style_text_font(_ampmLabel, &lv_font_montserrat_18, 0);
        lv_obj_set_style_text_color(_ampmLabel, lv_color_hex(0x888888), 0);
        
        // Date label
        _dateLabel = lv_label_create(getScreen());
        lv_label_set_text(_dateLabel, "-- --- ----");
        lv_obj_align(_dateLabel, LV_ALIGN_CENTER, 0, 0);
        lv_obj_set_style_text_font(_dateLabel, &lv_font_montserrat_16, 0);
        lv_obj_set_style_text_color(_dateLabel, lv_color_hex(0x333333), 0);
        
        // Day label
        _dayLabel = lv_label_create(getScreen());
        lv_label_set_text(_dayLabel, "--------");
        lv_obj_align(_dayLabel, LV_ALIGN_CENTER, 0, 25);
        lv_obj_set_style_text_font(_dayLabel, &lv_font_montserrat_14, 0);
        lv_obj_set_style_text_color(_dayLabel, lv_color_hex(0x888888), 0);
        
        // Day progress bar (shows how much of the day has passed)
        _dayProgress = lv_bar_create(getScreen());
        lv_obj_set_size(_dayProgress, 200, 8);
        lv_obj_align(_dayProgress, LV_ALIGN_CENTER, 0, 50);
        lv_bar_set_range(_dayProgress, 0, 86400); // 24 hours in seconds
        lv_obj_set_style_bg_color(_dayProgress, lv_color_hex(0xe5e7eb), 0);
        lv_obj_set_style_bg_color(_dayProgress, lv_color_hex(0x667eea), LV_PART_INDICATOR);
        lv_obj_set_style_radius(_dayProgress, 4, 0);
        lv_obj_set_style_border_width(_dayProgress, 0, 0);
        
        // Progress percentage label
        _progressLabel = lv_label_create(getScreen());
        lv_label_set_text(_progressLabel, "Day: 0%");
        lv_obj_align(_progressLabel, LV_ALIGN_CENTER, 0, 65);
        lv_obj_set_style_text_font(_progressLabel, &lv_font_montserrat_10, 0);
        lv_obj_set_style_text_color(_progressLabel, lv_color_hex(0x888888), 0);
        
        // Seconds bar (animated)
        _secondsBar = lv_bar_create(getScreen());
        lv_obj_set_size(_secondsBar, 200, 4);
        lv_obj_align(_secondsBar, LV_ALIGN_CENTER, 0, -20);
        lv_bar_set_range(_secondsBar, 0, 60);
        lv_obj_set_style_bg_color(_secondsBar, lv_color_hex(0xe5e7eb), 0);
        lv_obj_set_style_bg_color(_secondsBar, lv_color_hex(0x10b981), LV_PART_INDICATOR);
        lv_obj_set_style_radius(_secondsBar, 2, 0);
        lv_obj_set_style_border_width(_secondsBar, 0, 0);
        
        // Uptime
        _uptimeLabel = lv_label_create(getScreen());
        lv_label_set_text(_uptimeLabel, "Uptime: 0s");
        lv_obj_align(_uptimeLabel, LV_ALIGN_BOTTOM_MID, 0, -10);
        lv_obj_set_style_text_font(_uptimeLabel, &lv_font_montserrat_10, 0);
        lv_obj_set_style_text_color(_uptimeLabel, lv_color_hex(0x888888), 0);
        
        _lastUpdate = 0;
        _lastNtpSync = 0;
        _animPhase = 0;
        
        // Fade in animation
        lv_obj_set_style_opa(getScreen(), LV_OPA_0, 0);
        lv_obj_fade_in(getScreen(), 300, 0);
    }
    
    void onStart() override {
        log("Clock App started!");
        _ntpClient->forceUpdate();
    }
    
    void onUpdate() override {
        uint32_t now = millis();
        
        // NTP sync every 60 seconds - NON-BLOCKING
        if (now - _lastNtpSync >= 60000) {
            // Quick check without waiting
            if (_ntpClient->update()) {
                Serial.println("[Clock] NTP synced");
            }
            _lastNtpSync = now;
        }
        
        // Update display every 1000ms (every second is enough for a clock!)
        if (now - _lastUpdate >= 1000) {
            updateDisplay();
            _lastUpdate = now;
        }
        
        // Animate background separately at lower rate
        static uint32_t lastAnim = 0;
        if (now - lastAnim >= 50) {  // 20 FPS for background
            animateBackground();
            lastAnim = now;
        }
    }
    
    void onPause() override {
        log("Clock App paused");
    }
    
    void onDestroy() override {
        log("Clock App destroyed");
        
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
    
    uint32_t _lastUpdate;
    uint32_t _lastNtpSync;
    float _animPhase;
    
    void updateDisplay() {
        if (!_ntpClient) return;
        
        time_t epochTime = _ntpClient->getEpochTime();
        struct tm* timeInfo = localtime(&epochTime);
        
        // Format time (HH:MM:SS)
        int hour = timeInfo->tm_hour;
        const char* ampm = (hour >= 12) ? "PM" : "AM";
        if (hour > 12) hour -= 12;
        if (hour == 0) hour = 12;
        
        char timeBuf[16];
        snprintf(timeBuf, sizeof(timeBuf), "%02d:%02d:%02d", 
                 hour, timeInfo->tm_min, timeInfo->tm_sec);
        lv_label_set_text(_timeLabel, timeBuf);
        lv_label_set_text(_ampmLabel, ampm);
        
        // Update seconds bar with smooth animation
        lv_bar_set_value(_secondsBar, timeInfo->tm_sec, LV_ANIM_ON);
        
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
        
        // Day progress (seconds since midnight)
        int secondsSinceMidnight = timeInfo->tm_hour * 3600 + 
                                   timeInfo->tm_min * 60 + 
                                   timeInfo->tm_sec;
        lv_bar_set_value(_dayProgress, secondsSinceMidnight, LV_ANIM_ON);
        
        float dayPercent = (float)secondsSinceMidnight / 864.0f; // 86400 seconds / 100
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
    
    void animateBackground() {
        // Slow pulsing animation - only update every 100ms
        static uint32_t lastBgUpdate = 0;
        uint32_t now = millis();
        if (now - lastBgUpdate < 100) return;
        lastBgUpdate = now;
        
        _animPhase += 0.1f;
        if (_animPhase > 6.28f) _animPhase = 0;
        
        // Calculate opacity pulse (10% to 20%)
        uint8_t opa1 = 25 + (uint8_t)(15 * sin(_animPhase));
        uint8_t opa2 = 25 + (uint8_t)(15 * sin(_animPhase + 1.57f));
        
        lv_obj_set_style_bg_opa(_bgCircle1, opa1, 0);
        lv_obj_set_style_bg_opa(_bgCircle2, opa2, 0);
        
        // Subtle position shift
        int16_t shift1_x = -30 + (int16_t)(5 * sin(_animPhase * 0.5f));
        int16_t shift1_y = -40 + (int16_t)(5 * cos(_animPhase * 0.5f));
        int16_t shift2_x = 20 + (int16_t)(5 * sin(_animPhase * 0.3f + 1.0f));
        int16_t shift2_y = -30 + (int16_t)(5 * cos(_animPhase * 0.3f + 1.0f));
        
        lv_obj_align(_bgCircle1, LV_ALIGN_CENTER, shift1_x, shift1_y);
        lv_obj_align(_bgCircle2, LV_ALIGN_CENTER, shift2_x, shift2_y);
    }
};

#endif // CLOCK_APP_H