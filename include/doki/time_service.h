/**
 * TimeService - Centralized NTP Time Management for Doki OS
 *
 * Singleton service that manages NTP time synchronization across the entire system.
 * Replaces duplicate NTP clients in js_engine.cpp and clock_app.h.
 *
 * Benefits:
 * - Single NTP client (no duplicate UDP connections)
 * - No DNS errors from multiple clients
 * - Consistent time across all apps
 * - Background synchronization
 * - Thread-safe access
 *
 * Usage:
 *   TimeService& timeService = TimeService::getInstance();
 *   timeService.begin();
 *   if (timeService.isTimeSynced()) {
 *       struct tm time = timeService.getLocalTime();
 *   }
 */

#ifndef DOKI_TIME_SERVICE_H
#define DOKI_TIME_SERVICE_H

#include <Arduino.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
#include "hardware_config.h"
#include "timing_constants.h"

namespace Doki {

class TimeService {
public:
    /**
     * Get singleton instance
     */
    static TimeService& getInstance() {
        static TimeService instance;
        return instance;
    }

    /**
     * Initialize and start NTP synchronization
     * @return true if initialization successful
     */
    bool begin() {
        if (_initialized) {
            Serial.println("[TimeService] Already initialized");
            return true;
        }

        Serial.println("[TimeService] Initializing NTP client...");

        // Create UDP and NTP client
        _udp = new WiFiUDP();
        _ntpClient = new NTPClient(*_udp, "pool.ntp.org", 19800, UPDATE_INTERVAL_NTP_RETRY_MS);

        _ntpClient->begin();

        // Start background sync task
        xTaskCreatePinnedToCore(
            syncTask,
            "TimeService_Sync",
            TASK_STACK_NTP_SYNC,
            this,
            TASK_PRIORITY_NTP,
            &_syncTaskHandle,
            TASK_CORE_NETWORK
        );

        _initialized = true;
        Serial.println("[TimeService] ✓ Initialized (syncing in background)");
        return true;
    }

    /**
     * Check if time has been synced with NTP server
     */
    bool isTimeSynced() const {
        return _synced;
    }

    /**
     * Get current local time as struct tm
     * @return tm structure with current time, or zeros if not synced
     */
    struct tm getLocalTime() {
        struct tm timeInfo = {0};

        if (!_synced || !_ntpClient) {
            return timeInfo;
        }

        time_t epochTime = _ntpClient->getEpochTime();
        if (epochTime < 1000000000) {
            return timeInfo;  // Not valid yet
        }

        struct tm* timePtr = localtime(&epochTime);
        if (timePtr) {
            timeInfo = *timePtr;
        }

        return timeInfo;
    }

    /**
     * Get epoch time (seconds since 1970)
     */
    time_t getEpochTime() {
        if (!_synced || !_ntpClient) {
            return 0;
        }
        return _ntpClient->getEpochTime();
    }

    /**
     * Get formatted time string (HH:MM:SS)
     */
    String getTimeString(bool includeSeconds = true) {
        struct tm time = getLocalTime();
        if (time.tm_year == 0) return "--:--:--";

        char buffer[16];
        if (includeSeconds) {
            snprintf(buffer, sizeof(buffer), "%02d:%02d:%02d",
                    time.tm_hour, time.tm_min, time.tm_sec);
        } else {
            snprintf(buffer, sizeof(buffer), "%02d:%02d",
                    time.tm_hour, time.tm_min);
        }
        return String(buffer);
    }

    /**
     * Get formatted date string (DD/MM/YYYY)
     */
    String getDateString() {
        struct tm time = getLocalTime();
        if (time.tm_year == 0) return "--/--/----";

        char buffer[16];
        snprintf(buffer, sizeof(buffer), "%02d/%02d/%04d",
                time.tm_mday, time.tm_mon + 1, time.tm_year + 1900);
        return String(buffer);
    }

    /**
     * Cleanup (called automatically on shutdown)
     */
    ~TimeService() {
        if (_syncTaskHandle) {
            vTaskDelete(_syncTaskHandle);
        }
        if (_ntpClient) {
            delete _ntpClient;
        }
        if (_udp) {
            delete _udp;
        }
    }

private:
    // Singleton - prevent copying
    TimeService() : _ntpClient(nullptr), _udp(nullptr),
                   _syncTaskHandle(nullptr), _initialized(false), _synced(false) {}
    TimeService(const TimeService&) = delete;
    TimeService& operator=(const TimeService&) = delete;

    /**
     * Background task for NTP synchronization
     */
    static void syncTask(void* parameter) {
        TimeService* service = static_cast<TimeService*>(parameter);

        Serial.println("[TimeService] Background sync task started");

        // Wait for WiFi to stabilize
        vTaskDelay(pdMS_TO_TICKS(DELAY_NTP_INIT_MS));

        // Suppress WiFiUdp ESP-IDF errors
        esp_log_level_set("WiFiUdp", ESP_LOG_ERROR);

        // Initial sync
        Serial.println("[TimeService] Performing initial sync...");
        if (service->_ntpClient && service->_ntpClient->forceUpdate()) {
            service->_synced = true;
            Serial.println("[TimeService] ✓ Time synced successfully");
            Serial.println("[TimeService] Will update every 1 hour");
        } else {
            Serial.println("[TimeService] ✗ Initial sync failed, will retry");
        }

        // Periodic updates
        int failureCount = 0;
        const int MAX_FAILURES = 5;

        while (true) {
            // Wait interval based on sync status
            if (service->_synced) {
                vTaskDelay(pdMS_TO_TICKS(UPDATE_INTERVAL_NTP_MS));  // 1 hour
            } else {
                vTaskDelay(pdMS_TO_TICKS(UPDATE_INTERVAL_NTP_RETRY_MS));  // 1 minute
            }

            // Attempt update
            if (service->_ntpClient && service->_ntpClient->update()) {
                failureCount = 0;
                if (!service->_synced) {
                    service->_synced = true;
                    Serial.println("[TimeService] ✓ Time synced successfully");
                }
                // Silent success after first sync
            } else {
                failureCount++;
                if (!service->_synced || failureCount >= MAX_FAILURES) {
                    Serial.printf("[TimeService] Update failed (attempt %d)\n", failureCount);
                }
            }
        }
    }

    // Members
    NTPClient* _ntpClient;
    WiFiUDP* _udp;
    TaskHandle_t _syncTaskHandle;
    bool _initialized;
    volatile bool _synced;
};

} // namespace Doki

#endif // DOKI_TIME_SERVICE_H
