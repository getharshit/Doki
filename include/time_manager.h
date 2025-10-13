/**
 * Time Manager - Handles NTP sync and time formatting
 * Place in: include/time_manager.h
 */

#ifndef TIME_MANAGER_H
#define TIME_MANAGER_H

#include <Arduino.h>
#include <WiFi.h>
#include <time.h>
#include "config.h"

class TimeManager {
private:
    bool time_synced;
    unsigned long last_ntp_update;
    struct tm timeinfo;
    
public:
    TimeManager();
    
    // Initialize NTP time sync
    bool init();
    
    // Update time from NTP if needed
    void update();
    
    // Check if time is synchronized
    bool isSynced();
    
    // Get formatted time string (12h with AM/PM and seconds)
    String getTimeString();
    
    // Get formatted date string (DD/MM/YYYY)
    String getDateString();
    
    // Get day of week (Monday, Tuesday, etc.)
    String getDayOfWeek();
    
    // Get individual time components
    int getHour();      // 0-23
    int getHour12();    // 1-12
    int getMinute();    // 0-59
    int getSecond();    // 0-59
    bool isPM();        // true if PM
    
    // Get individual date components
    int getDay();       // 1-31
    int getMonth();     // 1-12
    int getYear();      // Full year (e.g., 2025)
    
    // Force NTP sync
    bool forceSync();
};

// Global instance
extern TimeManager timeManager;

#endif // TIME_MANAGER_H