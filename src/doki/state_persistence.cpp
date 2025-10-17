/**
 * @file state_persistence.cpp
 * @brief Implementation of State Persistence Module
 */

#include "doki/state_persistence.h"

namespace Doki {

// Static member initialization
bool StatePersistence::_initialized = false;
Preferences StatePersistence::_prefs;

bool StatePersistence::init() {
    if (_initialized) {
        Serial.println("[StatePersistence] Already initialized");
        return true;
    }

    Serial.println("[StatePersistence] Initializing...");

    // NVS is initialized by StorageManager, so we just mark as initialized
    _initialized = true;

    Serial.println("[StatePersistence] ✓ Initialized");
    return true;
}

bool StatePersistence::saveState(const char* appId, const JsonDocument& state) {
    if (!_initialized) {
        Serial.println("[StatePersistence] Error: Not initialized");
        return false;
    }

    if (!appId) {
        Serial.println("[StatePersistence] Error: Invalid app ID");
        return false;
    }

    // Serialize JSON to string
    String jsonString;
    serializeJson(state, jsonString);

    // Check size limit
    if (jsonString.length() > MAX_STATE_SIZE) {
        Serial.printf("[StatePersistence] Error: State too large (%d bytes, max %d)\n",
                      jsonString.length(), MAX_STATE_SIZE);
        return false;
    }

    // Open preferences
    if (!_prefs.begin(NAMESPACE, false)) {
        Serial.println("[StatePersistence] Error: Failed to open NVS");
        return false;
    }

    // Generate key
    String key = _makeKey(appId);

    // Save to NVS
    size_t written = _prefs.putString(key.c_str(), jsonString);
    _prefs.end();

    if (written == 0) {
        Serial.printf("[StatePersistence] Error: Failed to save state for '%s'\n", appId);
        return false;
    }

    Serial.printf("[StatePersistence] ✓ Saved state for '%s' (%d bytes)\n",
                  appId, jsonString.length());

    return true;
}

bool StatePersistence::loadState(const char* appId, JsonDocument& state) {
    if (!_initialized) {
        Serial.println("[StatePersistence] Error: Not initialized");
        return false;
    }

    if (!appId) {
        Serial.println("[StatePersistence] Error: Invalid app ID");
        return false;
    }

    // Open preferences
    if (!_prefs.begin(NAMESPACE, true)) { // Read-only
        Serial.println("[StatePersistence] Error: Failed to open NVS");
        return false;
    }

    // Generate key
    String key = _makeKey(appId);

    // Check if state exists
    if (!_prefs.isKey(key.c_str())) {
        _prefs.end();
        Serial.printf("[StatePersistence] No saved state for '%s'\n", appId);
        return false;
    }

    // Load from NVS
    String jsonString = _prefs.getString(key.c_str(), "");
    _prefs.end();

    if (jsonString.isEmpty()) {
        Serial.printf("[StatePersistence] Error: Empty state for '%s'\n", appId);
        return false;
    }

    // Deserialize JSON
    DeserializationError error = deserializeJson(state, jsonString);
    if (error) {
        Serial.printf("[StatePersistence] Error: Failed to parse state for '%s': %s\n",
                      appId, error.c_str());
        return false;
    }

    Serial.printf("[StatePersistence] ✓ Loaded state for '%s' (%d bytes)\n",
                  appId, jsonString.length());

    return true;
}

bool StatePersistence::hasState(const char* appId) {
    if (!_initialized || !appId) {
        return false;
    }

    if (!_prefs.begin(NAMESPACE, true)) {
        return false;
    }

    String key = _makeKey(appId);
    bool exists = _prefs.isKey(key.c_str());

    _prefs.end();

    return exists;
}

bool StatePersistence::clearState(const char* appId) {
    if (!_initialized) {
        Serial.println("[StatePersistence] Error: Not initialized");
        return false;
    }

    if (!appId) {
        Serial.println("[StatePersistence] Error: Invalid app ID");
        return false;
    }

    if (!_prefs.begin(NAMESPACE, false)) {
        Serial.println("[StatePersistence] Error: Failed to open NVS");
        return false;
    }

    String key = _makeKey(appId);
    bool removed = _prefs.remove(key.c_str());

    _prefs.end();

    if (removed) {
        Serial.printf("[StatePersistence] ✓ Cleared state for '%s'\n", appId);
    } else {
        Serial.printf("[StatePersistence] No state to clear for '%s'\n", appId);
    }

    return removed;
}

bool StatePersistence::clearAllStates() {
    if (!_initialized) {
        Serial.println("[StatePersistence] Error: Not initialized");
        return false;
    }

    if (!_prefs.begin(NAMESPACE, false)) {
        Serial.println("[StatePersistence] Error: Failed to open NVS");
        return false;
    }

    bool cleared = _prefs.clear();
    _prefs.end();

    if (cleared) {
        Serial.println("[StatePersistence] ✓ Cleared all states");
    } else {
        Serial.println("[StatePersistence] Error: Failed to clear states");
    }

    return cleared;
}

size_t StatePersistence::getStateSize(const char* appId) {
    if (!_initialized || !appId) {
        return 0;
    }

    if (!_prefs.begin(NAMESPACE, true)) {
        return 0;
    }

    String key = _makeKey(appId);
    size_t size = _prefs.getString(key.c_str(), "").length();

    _prefs.end();

    return size;
}

String StatePersistence::_makeKey(const char* appId) {
    // Simple key: "app_<appId>"
    String key = "app_";
    key += appId;
    return key;
}

} // namespace Doki
