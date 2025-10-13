// Time Manager Implementation
// Place in: src/time_manager.cpp

#include "time_manager.h"

TimeManager timeManager;

TimeManager::TimeManager() {
    time_synced = false;
    last_ntp_update = 0;
}

bool TimeManager::init() {
    Serial.println("Initializing NTP time sync...");
    configTime(GMT_OFFSET_SEC, DAYLIGHT_OFFSET_SEC, NTP_SERVER, NTP_SERVER_BACKUP);
    Serial.print("  Waiting for NTP sync");
    int retry = 0;
    while (!getLocalTime(&timeinfo) && retry < 20) {
        Serial.print(".");
        delay(500);
        retry++;
    }
    Serial.println();
    if (retry >= 20) {
        Serial.println("  ✗ Failed to sync with NTP server!");
        time_synced = false;
        return false;
    }
    time_synced = true;
    last_ntp_update = millis();
    Serial.println("  ✓ NTP time synchronized!");
    Serial.printf("  Current time: %s\n", getTimeString().c_str());
    Serial.printf("  Current date: %s\n", getDateString().c_str());
    return true;
}

void TimeManager::update() {
    if (time_synced && (millis() - last_ntp_update > NTP_UPDATE_INTERVAL_MS)) {
        Serial.println("Updating time from NTP...");
        forceSync();
    }
    getLocalTime(&timeinfo);
}

bool TimeManager::isSynced() {
    return time_synced;
}

String TimeManager::getTimeString() {
    if (!time_synced) return "--:--:--";
    getLocalTime(&timeinfo);
    char buffer[20];
    if (TIME_FORMAT_12H) {
        int hour12 = timeinfo.tm_hour % 12;
        if (hour12 == 0) hour12 = 12;
        const char* ampm = (timeinfo.tm_hour >= 12) ? "PM" : "AM";
        if (SHOW_SECONDS) {
            snprintf(buffer, sizeof(buffer), "%02d:%02d:%02d %s", hour12, timeinfo.tm_min, timeinfo.tm_sec, ampm);
        } else {
            snprintf(buffer, sizeof(buffer), "%02d:%02d %s", hour12, timeinfo.tm_min, ampm);
        }
    } else {
        if (SHOW_SECONDS) {
            snprintf(buffer, sizeof(buffer), "%02d:%02d:%02d", timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec);
        } else {
            snprintf(buffer, sizeof(buffer), "%02d:%02d", timeinfo.tm_hour, timeinfo.tm_min);
        }
    }
    return String(buffer);
}

String TimeManager::getDateString() {
    if (!time_synced) return "--/--/----";
    getLocalTime(&timeinfo);
    char buffer[20];
    if (DATE_FORMAT_DMY) {
        snprintf(buffer, sizeof(buffer), "%02d/%02d/%04d", timeinfo.tm_mday, timeinfo.tm_mon + 1, timeinfo.tm_year + 1900);
    } else {
        snprintf(buffer, sizeof(buffer), "%02d/%02d/%04d", timeinfo.tm_mon + 1, timeinfo.tm_mday, timeinfo.tm_year + 1900);
    }
    return String(buffer);
}

String TimeManager::getDayOfWeek() {
    if (!time_synced) return "---";
    getLocalTime(&timeinfo);
    const char* days[] = {"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"};
    return String(days[timeinfo.tm_wday]);
}

int TimeManager::getHour() {
    if (!time_synced) return 0;
    getLocalTime(&timeinfo);
    return timeinfo.tm_hour;
}

int TimeManager::getHour12() {
    if (!time_synced) return 0;
    getLocalTime(&timeinfo);
    int hour12 = timeinfo.tm_hour % 12;
    return (hour12 == 0) ? 12 : hour12;
}

int TimeManager::getMinute() {
    if (!time_synced) return 0;
    getLocalTime(&timeinfo);
    return timeinfo.tm_min;
}

int TimeManager::getSecond() {
    if (!time_synced) return 0;
    getLocalTime(&timeinfo);
    return timeinfo.tm_sec;
}

bool TimeManager::isPM() {
    if (!time_synced) return false;
    getLocalTime(&timeinfo);
    return timeinfo.tm_hour >= 12;
}

int TimeManager::getDay() {
    if (!time_synced) return 0;
    getLocalTime(&timeinfo);
    return timeinfo.tm_mday;
}

int TimeManager::getMonth() {
    if (!time_synced) return 0;
    getLocalTime(&timeinfo);
    return timeinfo.tm_mon + 1;
}

int TimeManager::getYear() {
    if (!time_synced) return 0;
    getLocalTime(&timeinfo);
    return timeinfo.tm_year + 1900;
}

bool TimeManager::forceSync() {
    Serial.println("Force syncing NTP...");
    configTime(GMT_OFFSET_SEC, DAYLIGHT_OFFSET_SEC, NTP_SERVER, NTP_SERVER_BACKUP);
    int retry = 0;
    while (!getLocalTime(&timeinfo) && retry < 10) {
        delay(500);
        retry++;
    }
    if (retry >= 10) {
        Serial.println("  ✗ NTP sync failed!");
        return false;
    }
    time_synced = true;
    last_ntp_update = millis();
    Serial.println("  ✓ NTP sync successful!");
    return true;
}