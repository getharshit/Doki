/**
 * @file sysinfo_app.h
 * @brief Enhanced System Info App with animated bars and graphs
 */

#ifndef SYSINFO_APP_H
#define SYSINFO_APP_H

#include "doki/app_base.h"
#include <WiFi.h>

class SysInfoApp : public Doki::DokiApp {
public:
    SysInfoApp() : DokiApp("sysinfo", "System Info") {}
    
    void onCreate() override {
        log("Creating Enhanced System Info App...");
        
        // Title
        _titleLabel = lv_label_create(getScreen());
        lv_label_set_text(_titleLabel, "System Info");
        lv_obj_align(_titleLabel, LV_ALIGN_TOP_MID, 0, 5);
        lv_obj_set_style_text_font(_titleLabel, &lv_font_montserrat_18, 0);
        lv_obj_set_style_text_color(_titleLabel, lv_color_hex(0x667eea), 0);
        
        // Version
        _versionLabel = lv_label_create(getScreen());
        lv_label_set_text(_versionLabel, "Doki OS v0.1.0");
        lv_obj_align(_versionLabel, LV_ALIGN_TOP_MID, 0, 30);
        lv_obj_set_style_text_font(_versionLabel, &lv_font_montserrat_10, 0);
        lv_obj_set_style_text_color(_versionLabel, lv_color_hex(0x888888), 0);
        
        // Uptime
        _uptimeLabel = lv_label_create(getScreen());
        lv_label_set_text(_uptimeLabel, "Uptime: 0s");
        lv_obj_align(_uptimeLabel, LV_ALIGN_TOP_LEFT, 10, 55);
        lv_obj_set_style_text_font(_uptimeLabel, &lv_font_montserrat_12, 0);
        
        // CPU Frequency
        _cpuLabel = lv_label_create(getScreen());
        lv_label_set_text(_cpuLabel, "CPU: 240 MHz");
        lv_obj_align(_cpuLabel, LV_ALIGN_TOP_RIGHT, -10, 55);
        lv_obj_set_style_text_font(_cpuLabel, &lv_font_montserrat_12, 0);
        
        // Heap Memory Bar
        _heapBar = lv_bar_create(getScreen());
        lv_obj_set_size(_heapBar, 220, 20);
        lv_obj_align(_heapBar, LV_ALIGN_TOP_MID, 0, 85);
        lv_bar_set_range(_heapBar, 0, 100);
        lv_obj_set_style_bg_color(_heapBar, lv_color_hex(0xe5e7eb), 0);
        lv_obj_set_style_bg_color(_heapBar, lv_color_hex(0xEF4444), LV_PART_INDICATOR);
        lv_obj_set_style_radius(_heapBar, 10, 0);
        
        _heapLabel = lv_label_create(getScreen());
        lv_label_set_text(_heapLabel, "Heap: 0/0 KB (0%)");
        lv_obj_align(_heapLabel, LV_ALIGN_TOP_MID, 0, 110);
        lv_obj_set_style_text_font(_heapLabel, &lv_font_montserrat_10, 0);
        
        // PSRAM Memory Bar
        _psramBar = lv_bar_create(getScreen());
        lv_obj_set_size(_psramBar, 220, 20);
        lv_obj_align(_psramBar, LV_ALIGN_TOP_MID, 0, 135);
        lv_bar_set_range(_psramBar, 0, 100);
        lv_obj_set_style_bg_color(_psramBar, lv_color_hex(0xe5e7eb), 0);
        lv_obj_set_style_bg_color(_psramBar, lv_color_hex(0x3B82F6), LV_PART_INDICATOR);
        lv_obj_set_style_radius(_psramBar, 10, 0);
        
        _psramLabel = lv_label_create(getScreen());
        lv_label_set_text(_psramLabel, "PSRAM: 0/0 KB (0%)");
        lv_obj_align(_psramLabel, LV_ALIGN_TOP_MID, 0, 160);
        lv_obj_set_style_text_font(_psramLabel, &lv_font_montserrat_10, 0);
        
        // WiFi Section
        _wifiLabel = lv_label_create(getScreen());
        lv_label_set_text(_wifiLabel, "WiFi: Checking...");
        lv_obj_align(_wifiLabel, LV_ALIGN_TOP_LEFT, 10, 190);
        lv_obj_set_style_text_font(_wifiLabel, &lv_font_montserrat_12, 0);
        
        // WiFi Signal Bar
        _signalBar = lv_bar_create(getScreen());
        lv_obj_set_size(_signalBar, 220, 12);
        lv_obj_align(_signalBar, LV_ALIGN_TOP_MID, 0, 210);
        lv_bar_set_range(_signalBar, -100, -30); // dBm range
        lv_obj_set_style_bg_color(_signalBar, lv_color_hex(0xe5e7eb), 0);
        lv_obj_set_style_bg_color(_signalBar, lv_color_hex(0x10b981), LV_PART_INDICATOR);
        lv_obj_set_style_radius(_signalBar, 6, 0);
        
        _signalLabel = lv_label_create(getScreen());
        lv_label_set_text(_signalLabel, "Signal: -- dBm");
        lv_obj_align(_signalLabel, LV_ALIGN_TOP_MID, 0, 228);
        lv_obj_set_style_text_font(_signalLabel, &lv_font_montserrat_10, 0);
        
        // Temperature (CPU)
        _tempLabel = lv_label_create(getScreen());
        lv_label_set_text(_tempLabel, "Temp: --°C");
        lv_obj_align(_tempLabel, LV_ALIGN_TOP_LEFT, 10, 255);
        lv_obj_set_style_text_font(_tempLabel, &lv_font_montserrat_12, 0);
        
        // App Uptime
        _appUptimeLabel = lv_label_create(getScreen());
        lv_label_set_text(_appUptimeLabel, "App: 0s");
        lv_obj_align(_appUptimeLabel, LV_ALIGN_TOP_RIGHT, -10, 255);
        lv_obj_set_style_text_font(_appUptimeLabel, &lv_font_montserrat_12, 0);
        
        _lastUpdate = 0;
        
        // Fade in
        lv_obj_set_style_opa(getScreen(), LV_OPA_0, 0);
        lv_obj_fade_in(getScreen(), 350, 0);
    }
    
    void onStart() override {
        log("System Info App started!");
    }
    
    void onUpdate() override {
        uint32_t now = millis();
        
        // Update every second
        if (now - _lastUpdate >= 1000) {
            updateDisplay();
            _lastUpdate = now;
        }
    }
    
    void onPause() override {
        log("System Info App paused");
    }
    
    void onDestroy() override {
        log("System Info App destroyed");
    }

private:
    lv_obj_t* _titleLabel;
    lv_obj_t* _versionLabel;
    lv_obj_t* _uptimeLabel;
    lv_obj_t* _cpuLabel;
    lv_obj_t* _heapBar;
    lv_obj_t* _heapLabel;
    lv_obj_t* _psramBar;
    lv_obj_t* _psramLabel;
    lv_obj_t* _wifiLabel;
    lv_obj_t* _signalBar;
    lv_obj_t* _signalLabel;
    lv_obj_t* _tempLabel;
    lv_obj_t* _appUptimeLabel;
    
    uint32_t _lastUpdate;
    
    void updateDisplay() {
        // System uptime
        uint32_t uptime = millis() / 1000;
        char uptimeBuf[32];
        if (uptime < 60) {
            snprintf(uptimeBuf, sizeof(uptimeBuf), "Uptime: %lus", uptime);
        } else if (uptime < 3600) {
            snprintf(uptimeBuf, sizeof(uptimeBuf), "Uptime: %lum", uptime / 60);
        } else {
            snprintf(uptimeBuf, sizeof(uptimeBuf), "Uptime: %luh %lum", 
                     uptime / 3600, (uptime % 3600) / 60);
        }
        lv_label_set_text(_uptimeLabel, uptimeBuf);
        
        // CPU info
        uint32_t cpuFreq = ESP.getCpuFreqMHz();
        char cpuBuf[32];
        snprintf(cpuBuf, sizeof(cpuBuf), "CPU: %lu MHz", cpuFreq);
        lv_label_set_text(_cpuLabel, cpuBuf);
        
        // Memory - Heap
        uint32_t freeHeap = ESP.getFreeHeap();
        uint32_t totalHeap = ESP.getHeapSize();
        uint32_t usedHeap = totalHeap - freeHeap;
        int heapPercent = (int)((float)usedHeap / totalHeap * 100);
        
        char heapBuf[48];
        snprintf(heapBuf, sizeof(heapBuf), "Heap: %lu/%lu KB (%d%%)",
                 usedHeap / 1024, totalHeap / 1024, heapPercent);
        lv_label_set_text(_heapLabel, heapBuf);
        lv_bar_set_value(_heapBar, heapPercent, LV_ANIM_ON);
        
        // Color based on usage
        if (heapPercent > 80) {
            lv_obj_set_style_bg_color(_heapBar, lv_color_hex(0xEF4444), LV_PART_INDICATOR);
        } else if (heapPercent > 60) {
            lv_obj_set_style_bg_color(_heapBar, lv_color_hex(0xF59E0B), LV_PART_INDICATOR);
        } else {
            lv_obj_set_style_bg_color(_heapBar, lv_color_hex(0x10b981), LV_PART_INDICATOR);
        }
        
        // Memory - PSRAM
        uint32_t freePsram = ESP.getFreePsram();
        uint32_t totalPsram = ESP.getPsramSize();
        uint32_t usedPsram = totalPsram - freePsram;
        int psramPercent = (int)((float)usedPsram / totalPsram * 100);
        
        char psramBuf[48];
        snprintf(psramBuf, sizeof(psramBuf), "PSRAM: %lu/%lu KB (%d%%)",
                 usedPsram / 1024, totalPsram / 1024, psramPercent);
        lv_label_set_text(_psramLabel, psramBuf);
        lv_bar_set_value(_psramBar, psramPercent, LV_ANIM_ON);
        
        // WiFi
        bool connected = (WiFi.status() == WL_CONNECTED);
        if (connected) {
            String ssid = WiFi.SSID();
            if (ssid.length() > 15) ssid = ssid.substring(0, 15);
            
            char wifiBuf[32];
            snprintf(wifiBuf, sizeof(wifiBuf), "WiFi: %s", ssid.c_str());
            lv_label_set_text(_wifiLabel, wifiBuf);
            
            int rssi = WiFi.RSSI();
            lv_bar_set_value(_signalBar, rssi, LV_ANIM_ON);
            
            char signalBuf[32];
            snprintf(signalBuf, sizeof(signalBuf), "Signal: %d dBm", rssi);
            lv_label_set_text(_signalLabel, signalBuf);
            
            // Color based on signal strength
            if (rssi > -50) {
                lv_obj_set_style_bg_color(_signalBar, lv_color_hex(0x10b981), LV_PART_INDICATOR);
            } else if (rssi > -70) {
                lv_obj_set_style_bg_color(_signalBar, lv_color_hex(0xF59E0B), LV_PART_INDICATOR);
            } else {
                lv_obj_set_style_bg_color(_signalBar, lv_color_hex(0xEF4444), LV_PART_INDICATOR);
            }
        } else {
            lv_label_set_text(_wifiLabel, "WiFi: Disconnected");
            lv_bar_set_value(_signalBar, -100, LV_ANIM_OFF);
            lv_label_set_text(_signalLabel, "Signal: N/A");
        }
        
        // Temperature
        float temp = temperatureRead();
        char tempBuf[32];
        snprintf(tempBuf, sizeof(tempBuf), "Temp: %.1f°C", temp);
        lv_label_set_text(_tempLabel, tempBuf);
        
        // App uptime
        uint32_t appUptime = getUptime() / 1000;
        char appBuf[32];
        if (appUptime < 60) {
            snprintf(appBuf, sizeof(appBuf), "App: %lus", appUptime);
        } else if (appUptime < 3600) {
            snprintf(appBuf, sizeof(appBuf), "App: %lum", appUptime / 60);
        } else {
            snprintf(appBuf, sizeof(appBuf), "App: %luh", appUptime / 3600);
        }
        lv_label_set_text(_appUptimeLabel, appBuf);
    }
};

#endif // SYSINFO_APP_H