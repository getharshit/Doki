/**
 * @file storage_manager.h
 * @brief Storage Manager for Doki OS - Persistent data storage using NVS
 *
 * Provides a clean interface for storing and retrieving persistent data
 * such as WiFi credentials, app preferences, and configuration settings.
 *
 * Uses ESP32's NVS (Non-Volatile Storage) for data persistence across reboots.
 *
 * Example:
 *   StorageManager::init();
 *   StorageManager::saveWiFiCredentials("MyWiFi", "password123");
 *
 *   String ssid, password;
 *   if (StorageManager::loadWiFiCredentials(ssid, password)) {
 *       WiFi.begin(ssid.c_str(), password.c_str());
 *   }
 */

#ifndef DOKI_STORAGE_MANAGER_H
#define DOKI_STORAGE_MANAGER_H

#include <Arduino.h>
#include <Preferences.h>

namespace Doki {

/**
 * @brief Storage Manager - Persistent data storage using NVS
 *
 * Singleton class that manages all persistent storage operations
 */
class StorageManager {
public:
    /**
     * @brief Initialize storage manager
     *
     * @return true if initialization succeeded
     *
     * Must be called once during setup before using any storage functions.
     *
     * Example:
     *   void setup() {
     *       StorageManager::init();
     *   }
     */
    static bool init();

    // ========================================
    // WiFi Credentials Storage
    // ========================================

    /**
     * @brief Save WiFi credentials
     *
     * @param ssid WiFi network name
     * @param password WiFi password
     * @return true if save succeeded
     *
     * Stores WiFi credentials persistently. Will survive reboots.
     *
     * Example:
     *   StorageManager::saveWiFiCredentials("MyHomeWiFi", "password123");
     */
    static bool saveWiFiCredentials(const String& ssid, const String& password);

    /**
     * @brief Load WiFi credentials
     *
     * @param ssid Output: WiFi network name
     * @param password Output: WiFi password
     * @return true if credentials were loaded successfully
     *
     * Returns false if no credentials are saved.
     *
     * Example:
     *   String ssid, password;
     *   if (StorageManager::loadWiFiCredentials(ssid, password)) {
     *       Serial.printf("Loaded WiFi: %s\n", ssid.c_str());
     *   } else {
     *       Serial.println("No WiFi credentials saved");
     *   }
     */
    static bool loadWiFiCredentials(String& ssid, String& password);

    /**
     * @brief Check if WiFi credentials are saved
     *
     * @return true if credentials exist
     *
     * Quick check without loading the actual values.
     */
    static bool hasWiFiCredentials();

    /**
     * @brief Clear WiFi credentials
     *
     * @return true if clear succeeded
     *
     * Removes saved WiFi credentials from storage.
     */
    static bool clearWiFiCredentials();

    // ========================================
    // Configuration Storage
    // ========================================

    /**
     * @brief Save string value
     *
     * @param key Configuration key
     * @param value String value to save
     * @return true if save succeeded
     *
     * Example:
     *   StorageManager::setString("last_app", "clock");
     */
    static bool setString(const char* key, const String& value);

    /**
     * @brief Load string value
     *
     * @param key Configuration key
     * @param defaultValue Default value if key not found
     * @return Stored value or default value
     *
     * Example:
     *   String lastApp = StorageManager::getString("last_app", "weather");
     */
    static String getString(const char* key, const String& defaultValue = "");

    /**
     * @brief Save integer value
     *
     * @param key Configuration key
     * @param value Integer value to save
     * @return true if save succeeded
     */
    static bool setInt(const char* key, int value);

    /**
     * @brief Load integer value
     *
     * @param key Configuration key
     * @param defaultValue Default value if key not found
     * @return Stored value or default value
     */
    static int getInt(const char* key, int defaultValue = 0);

    /**
     * @brief Save boolean value
     *
     * @param key Configuration key
     * @param value Boolean value to save
     * @return true if save succeeded
     */
    static bool setBool(const char* key, bool value);

    /**
     * @brief Load boolean value
     *
     * @param key Configuration key
     * @param defaultValue Default value if key not found
     * @return Stored value or default value
     */
    static bool getBool(const char* key, bool defaultValue = false);

    // ========================================
    // Utility Functions
    // ========================================

    /**
     * @brief Check if a key exists
     *
     * @param key Configuration key
     * @return true if key exists
     */
    static bool hasKey(const char* key);

    /**
     * @brief Remove a specific key
     *
     * @param key Configuration key
     * @return true if removal succeeded
     */
    static bool remove(const char* key);

    /**
     * @brief Clear all stored data
     *
     * @return true if clear succeeded
     *
     * WARNING: This removes ALL data including WiFi credentials!
     */
    static bool clearAll();

    /**
     * @brief Get free storage space
     *
     * @return Free entries in NVS
     */
    static size_t getFreeSpace();

    /**
     * @brief Print storage statistics
     *
     * Outputs storage usage info to Serial
     */
    static void printStats();

    /**
     * @brief Check if storage is initialized
     *
     * @return true if initialized
     */
    static bool isInitialized();

private:
    static Preferences _prefs;
    static bool _initialized;
    static const char* NAMESPACE;

    // Key names for internal use
    static const char* KEY_WIFI_SSID;
    static const char* KEY_WIFI_PASSWORD;
    static const char* KEY_WIFI_CONFIGURED;
};

} // namespace Doki

#endif // DOKI_STORAGE_MANAGER_H
