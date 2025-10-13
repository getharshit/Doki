/**
 * @file clock_app.h
 * @brief Clock App with NTP time sync for Doki OS
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
        log("Creating Clock App UI...");
        
        // Initialize NTP client
        _udp = new WiFiUDP();
        // India timezone: UTC+5:30 = 19800 seconds
        _ntpClient = new NTPClient(*_udp, "pool.ntp.org", 19800, 60000);
        _ntpClient->begin();
        
        log("NTP client initialized");
        
        // Create time label (large)
        _timeLabel = lv_label_create(getScreen());
        lv_label_set_text(_timeLabel, "--:--:-- --");
        lv_obj_align(_timeLabel, LV_ALIGN_CENTER, 0, -30);
        lv_obj_set_style_text_font(_timeLabel, &lv_font_montserrat_32, 0);
        lv_obj_set_style_text_color(_timeLabel, lv_color_hex(0x667eea), 0);
        
        // Create date label
        _dateLabel = lv_label_create(getScreen());
        lv_label_set_text(_dateLabel, "-- --- ----");
        lv_obj_align(_dateLabel, LV_ALIGN_CENTER, 0, 10);
        lv_obj_set_style_text_font(_dateLabel, &lv_font_montserrat_16, 0);
        
        // Create day label
        _dayLabel = lv_label_create(getScreen());
        lv_label_set_text(_dayLabel, "---------");
        lv_obj_align(_dayLabel, LV_ALIGN_CENTER, 0, 35);
        lv_obj_set_style_text_font(_dayLabel, &lv_font_montserrat_14, 0);
        lv_obj_set_style_text_color(_dayLabel, lv_color_hex(0x888888), 0);
        
        // Create uptime label
        _uptimeLabel = lv_label_create(getScreen());
        lv_label_set_text(_uptimeLabel, "Uptime: 0s");
        lv_obj_align(_uptimeLabel, LV_ALIGN_BOTTOM_MID, 0, -10);
        lv_obj_set_style_text_font(_uptimeLabel, &lv_font_montserrat_10, 0);
        lv_obj_set_style_text_color(_uptimeLabel, lv_color_hex(0x888888), 0);
        
        _lastUpdate = 0;
        _lastNtpSync = 0;
    }
    
    void onStart() override {
        log("Clock App started!");
        // Force immediate NTP sync
        _ntpClient->forceUpdate();
        log("Initial NTP sync requested");
    }
    
    void onUpdate() override {
        uint32_t now = millis();
        
        // Sync NTP every 60 seconds
        if (now - _lastNtpSync >= 60000) {
            _ntpClient->update();
            _lastNtpSync = now;
        }
        
        // Update display every second
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
        
        // Stop NTP client
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
    lv_obj_t* _timeLabel;
    lv_obj_t* _dateLabel;
    lv_obj_t* _dayLabel;
    lv_obj_t* _uptimeLabel;
    
    NTPClient* _ntpClient;
    WiFiUDP* _udp;
    
    uint32_t _lastUpdate;
    uint32_t _lastNtpSync;
    
    void updateDisplay() {
        if (!_ntpClient) return;
        
        // Get time from NTP
        time_t epochTime = _ntpClient->getEpochTime();
        struct tm* timeInfo = localtime(&epochTime);
        
        // Format time (12-hour with AM/PM)
        char timeBuf[32];
        int hour = timeInfo->tm_hour;
        const char* ampm = "AM";
        if (hour >= 12) {
            ampm = "PM";
            if (hour > 12) hour -= 12;
        }
        if (hour == 0) hour = 12;
        
        snprintf(timeBuf, sizeof(timeBuf), "%02d:%02d:%02d %s",
                 hour, timeInfo->tm_min, timeInfo->tm_sec, ampm);
        lv_label_set_text(_timeLabel, timeBuf);
        
        // Format date (DD MMM YYYY)
        const char* months[] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun",
                               "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};
        char dateBuf[32];
        snprintf(dateBuf, sizeof(dateBuf), "%02d %s %d",
                 timeInfo->tm_mday, months[timeInfo->tm_mon], 
                 timeInfo->tm_year + 1900);
        lv_label_set_text(_dateLabel, dateBuf);
        
        // Format day
        const char* days[] = {"Sunday", "Monday", "Tuesday", "Wednesday",
                             "Thursday", "Friday", "Saturday"};
        lv_label_set_text(_dayLabel, days[timeInfo->tm_wday]);
        
        // Update uptime
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