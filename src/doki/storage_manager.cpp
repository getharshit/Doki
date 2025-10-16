/**
 * @file storage_manager.cpp
 * @brief Implementation of StorageManager
 */

#include "doki/storage_manager.h"

namespace Doki {

// ========================================
// Static Member Initialization
// ========================================

Preferences StorageManager::_prefs;
bool StorageManager::_initialized = false;

// Namespace for NVS partition
const char* StorageManager::NAMESPACE = "doki";

// Key names
const char* StorageManager::KEY_WIFI_SSID = "wifi_ssid";
const char* StorageManager::KEY_WIFI_PASSWORD = "wifi_pass";
const char* StorageManager::KEY_WIFI_CONFIGURED = "wifi_cfg";

// ========================================
// Initialization
// ========================================

bool StorageManager::init() {
    if (_initialized) {
        Serial.println("[StorageManager] Already initialized");
        return true;
    }

    Serial.println("╔═══════════════════════════════════════════════════╗");
    Serial.println("║        Storage Manager Initialization             ║");
    Serial.println("╚═══════════════════════════════════════════════════╝");

    // Initialize NVS
    if (!_prefs.begin(NAMESPACE, false)) {
        Serial.println("[StorageManager] ✗ Failed to initialize NVS!");
        return false;
    }

    _initialized = true;

    Serial.println("[StorageManager] ✓ NVS initialized successfully");
    printStats();

    return true;
}

// ========================================
// WiFi Credentials Storage
// ========================================

bool StorageManager::saveWiFiCredentials(const String& ssid, const String& password) {
    if (!_initialized) {
        Serial.println("[StorageManager] Error: Not initialized!");
        return false;
    }

    if (ssid.isEmpty()) {
        Serial.println("[StorageManager] Error: SSID cannot be empty");
        return false;
    }

    Serial.printf("[StorageManager] Saving WiFi credentials: %s\n", ssid.c_str());

    // Save SSID
    if (!_prefs.putString(KEY_WIFI_SSID, ssid)) {
        Serial.println("[StorageManager] ✗ Failed to save SSID");
        return false;
    }

    // Save password (empty string for open WiFi is allowed)
    // Use a space character if password is truly empty to avoid putString issues
    String passwordToSave = password.isEmpty() ? " " : password;
    if (!_prefs.putString(KEY_WIFI_PASSWORD, passwordToSave)) {
        Serial.println("[StorageManager] ✗ Failed to save password");
        return false;
    }

    // Mark as configured
    if (!_prefs.putBool(KEY_WIFI_CONFIGURED, true)) {
        Serial.println("[StorageManager] ✗ Failed to save configuration flag");
        return false;
    }

    if (password.isEmpty()) {
        Serial.println("[StorageManager] ✓ WiFi credentials saved (open network)");
    } else {
        Serial.println("[StorageManager] ✓ WiFi credentials saved successfully");
    }
    return true;
}

bool StorageManager::loadWiFiCredentials(String& ssid, String& password) {
    if (!_initialized) {
        Serial.println("[StorageManager] Error: Not initialized!");
        return false;
    }

    // Check if configured
    if (!_prefs.getBool(KEY_WIFI_CONFIGURED, false)) {
        Serial.println("[StorageManager] No WiFi credentials configured");
        return false;
    }

    // Load SSID
    ssid = _prefs.getString(KEY_WIFI_SSID, "");
    if (ssid.isEmpty()) {
        Serial.println("[StorageManager] ✗ SSID is empty");
        return false;
    }

    // Load password (convert space back to empty string for open networks)
    password = _prefs.getString(KEY_WIFI_PASSWORD, "");
    if (password == " ") {
        password = "";  // Convert back to empty for open WiFi
    }

    if (password.isEmpty()) {
        Serial.printf("[StorageManager] ✓ Loaded WiFi credentials: %s (open network)\n", ssid.c_str());
    } else {
        Serial.printf("[StorageManager] ✓ Loaded WiFi credentials: %s\n", ssid.c_str());
    }
    return true;
}

bool StorageManager::hasWiFiCredentials() {
    if (!_initialized) return false;
    return _prefs.getBool(KEY_WIFI_CONFIGURED, false);
}

bool StorageManager::clearWiFiCredentials() {
    if (!_initialized) {
        Serial.println("[StorageManager] Error: Not initialized!");
        return false;
    }

    Serial.println("[StorageManager] Clearing WiFi credentials...");

    _prefs.remove(KEY_WIFI_SSID);
    _prefs.remove(KEY_WIFI_PASSWORD);
    _prefs.remove(KEY_WIFI_CONFIGURED);

    Serial.println("[StorageManager] ✓ WiFi credentials cleared");
    return true;
}

// ========================================
// Configuration Storage
// ========================================

bool StorageManager::setString(const char* key, const String& value) {
    if (!_initialized) return false;
    return _prefs.putString(key, value) > 0;
}

String StorageManager::getString(const char* key, const String& defaultValue) {
    if (!_initialized) return defaultValue;
    return _prefs.getString(key, defaultValue);
}

bool StorageManager::setInt(const char* key, int value) {
    if (!_initialized) return false;
    return _prefs.putInt(key, value) > 0;
}

int StorageManager::getInt(const char* key, int defaultValue) {
    if (!_initialized) return defaultValue;
    return _prefs.getInt(key, defaultValue);
}

bool StorageManager::setBool(const char* key, bool value) {
    if (!_initialized) return false;
    return _prefs.putBool(key, value);
}

bool StorageManager::getBool(const char* key, bool defaultValue) {
    if (!_initialized) return defaultValue;
    return _prefs.getBool(key, defaultValue);
}

// ========================================
// Utility Functions
// ========================================

bool StorageManager::hasKey(const char* key) {
    if (!_initialized) return false;
    return _prefs.isKey(key);
}

bool StorageManager::remove(const char* key) {
    if (!_initialized) return false;
    return _prefs.remove(key);
}

bool StorageManager::clearAll() {
    if (!_initialized) return false;

    Serial.println("[StorageManager] WARNING: Clearing ALL stored data!");
    return _prefs.clear();
}

size_t StorageManager::getFreeSpace() {
    if (!_initialized) return 0;
    return _prefs.freeEntries();
}

void StorageManager::printStats() {
    if (!_initialized) {
        Serial.println("[StorageManager] Not initialized");
        return;
    }

    Serial.println("\n┌─────────────────────────────────────────┐");
    Serial.println("│         Storage Statistics              │");
    Serial.println("├─────────────────────────────────────────┤");
    Serial.printf("│ Free entries:     %-18zu │\n", getFreeSpace());
    Serial.printf("│ WiFi configured:  %-18s │\n",
                  hasWiFiCredentials() ? "Yes" : "No");

    if (hasWiFiCredentials()) {
        String ssid = _prefs.getString(KEY_WIFI_SSID, "");
        Serial.printf("│ WiFi SSID:        %-18s │\n", ssid.c_str());
    }

    Serial.println("└─────────────────────────────────────────┘\n");
}

bool StorageManager::isInitialized() {
    return _initialized;
}

} // namespace Doki
