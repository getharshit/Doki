/**
 * @file sysinfo_app.h
 * @brief System Info App - Display system stats for Doki OS
 */

#ifndef SYSINFO_APP_H
#define SYSINFO_APP_H

#include "doki/app_base.h"
#include "doki/memory_manager.h"
#include "doki/task_scheduler.h"
#include "doki/app_manager.h"
#include <WiFi.h>

class SysInfoApp : public Doki::DokiApp {
public:
    SysInfoApp() : DokiApp("sysinfo", "System Info") {}
    
    void onCreate() override {
        log("Creating System Info App UI...");
        
        // Title
        _titleLabel = lv_label_create(getScreen());
        lv_label_set_text(_titleLabel, "System Info");
        lv_obj_align(_titleLabel, LV_ALIGN_TOP_MID, 0, 5);
        lv_obj_set_style_text_font(_titleLabel, &lv_font_montserrat_18, 0);
        lv_obj_set_style_text_color(_titleLabel, lv_color_hex(0x667eea), 0);
        
        // Create info container
        _infoLabel = lv_label_create(getScreen());
        lv_label_set_text(_infoLabel, "Loading...");
        lv_obj_align(_infoLabel, LV_ALIGN_TOP_LEFT, 10, 35);
        lv_obj_set_style_text_font(_infoLabel, &lv_font_montserrat_10, 0);
        lv_label_set_long_mode(_infoLabel, LV_LABEL_LONG_WRAP);
        lv_obj_set_width(_infoLabel, 240 - 20);
        
        _lastUpdate = 0;
    }
    
    void onStart() override {
        log("System Info App started!");
    }
    
    void onUpdate() override {
        // Update every 2 seconds
        uint32_t now = millis();
        if (now - _lastUpdate >= 2000) {
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
    lv_obj_t* _infoLabel;
    uint32_t _lastUpdate;
    
    void updateDisplay() {
        // Get system info
        Doki::SystemMemory mem = Doki::MemoryManager::getSystemMemory();
        uint32_t uptime = millis() / 1000;
        int activeTasks = Doki::TaskScheduler::getActiveTaskCount();
        const char* currentApp = Doki::AppManager::getCurrentAppId();
        uint32_t appUptime = Doki::AppManager::getAppUptime() / 1000;
        
        // WiFi info
        bool wifiConnected = (WiFi.status() == WL_CONNECTED);
        String wifiSsid = WiFi.SSID();
        int wifiRssi = WiFi.RSSI();
        String wifiIp = WiFi.localIP().toString();
        
        // CPU info
        uint32_t cpuFreq = ESP.getCpuFreqMHz();
        float cpuTemp = temperatureRead();  // Internal sensor
        
        // Format uptime
        char uptimeBuf[32];
        if (uptime < 60) {
            snprintf(uptimeBuf, sizeof(uptimeBuf), "%lus", uptime);
        } else if (uptime < 3600) {
            snprintf(uptimeBuf, sizeof(uptimeBuf), "%lum %lus", 
                     uptime / 60, uptime % 60);
        } else {
            snprintf(uptimeBuf, sizeof(uptimeBuf), "%luh %lum", 
                     uptime / 3600, (uptime % 3600) / 60);
        }
        
        // Format app uptime
        char appUptimeBuf[32];
        if (appUptime < 60) {
            snprintf(appUptimeBuf, sizeof(appUptimeBuf), "%lus", appUptime);
        } else if (appUptime < 3600) {
            snprintf(appUptimeBuf, sizeof(appUptimeBuf), "%lum %lus", 
                     appUptime / 60, appUptime % 60);
        } else {
            snprintf(appUptimeBuf, sizeof(appUptimeBuf), "%luh %lum", 
                     appUptime / 3600, (appUptime % 3600) / 60);
        }
        
        // Build info string
        char infoBuf[512];
        snprintf(infoBuf, sizeof(infoBuf),
            "DOKI OS v0.1.0\n"
            "\n"
            "System:\n"
            "  Uptime: %s\n"
            "  CPU: %lu MHz (%.1f°C)\n"
            "  Tasks: %d active\n"
            "\n"
            "Memory:\n"
            "  Heap: %d/%d KB (%d%%)\n"
            "  PSRAM: %d/%d KB (%d%%)\n"
            "\n"
            "WiFi:\n"
            "  %s\n"
            "  %s (%d dBm)\n"
            "  IP: %s\n"
            "\n"
            "Current App:\n"
            "  %s (%s)\n",
            uptimeBuf,
            cpuFreq, cpuTemp,
            activeTasks,
            (mem.totalHeap - mem.freeHeap) / 1024, mem.totalHeap / 1024,
            (int)(mem.heapUsagePercent * 100),
            (mem.totalPsram - mem.freePsram) / 1024, mem.totalPsram / 1024,
            (int)(mem.psramUsagePercent * 100),
            wifiConnected ? "Connected" : "Disconnected",
            wifiSsid.c_str(), wifiRssi,
            wifiIp.c_str(),
            currentApp, appUptimeBuf
        );
        
        lv_label_set_text(_infoLabel, infoBuf);
    }
};

#endif // SYSINFO_APP_H