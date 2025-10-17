/**
 * @file state_persistence.h
 * @brief State Persistence Module for Doki OS
 *
 * Allows apps to save and restore their state across unload/load cycles.
 * Uses NVS (Non-Volatile Storage) for persistent storage.
 *
 * Example:
 *   // In onPause():
 *   JsonDocument state;
 *   state["lastUpdate"] = millis();
 *   StatePersistence::saveState("myapp", state);
 *
 *   // In onCreate():
 *   JsonDocument state;
 *   if (StatePersistence::loadState("myapp", state)) {
 *       uint32_t lastUpdate = state["lastUpdate"];
 *   }
 */

#ifndef DOKI_STATE_PERSISTENCE_H
#define DOKI_STATE_PERSISTENCE_H

#include <Arduino.h>
#include <ArduinoJson.h>
#include <Preferences.h>

namespace Doki {

/**
 * @brief State Persistence Manager
 *
 * Handles saving and loading app state to/from NVS
 */
class StatePersistence {
public:
    /**
     * @brief Initialize state persistence system
     * @return true if initialized successfully
     */
    static bool init();

    /**
     * @brief Save app state to NVS
     *
     * @param appId App unique identifier
     * @param state JSON document containing state data
     * @return true if saved successfully
     *
     * Example:
     *   JsonDocument state;
     *   state["temperature"] = 25.5;
     *   state["lastFetch"] = millis();
     *   StatePersistence::saveState("weather", state);
     */
    static bool saveState(const char* appId, const JsonDocument& state);

    /**
     * @brief Load app state from NVS
     *
     * @param appId App unique identifier
     * @param state JSON document to populate with loaded state
     * @return true if state was found and loaded
     *
     * Example:
     *   JsonDocument state;
     *   if (StatePersistence::loadState("weather", state)) {
     *       float temp = state["temperature"];
     *   }
     */
    static bool loadState(const char* appId, JsonDocument& state);

    /**
     * @brief Check if state exists for an app
     *
     * @param appId App unique identifier
     * @return true if state exists in NVS
     */
    static bool hasState(const char* appId);

    /**
     * @brief Clear saved state for an app
     *
     * @param appId App unique identifier
     * @return true if cleared successfully
     */
    static bool clearState(const char* appId);

    /**
     * @brief Clear all saved states
     *
     * @return true if cleared successfully
     */
    static bool clearAllStates();

    /**
     * @brief Get size of saved state
     *
     * @param appId App unique identifier
     * @return Size in bytes, or 0 if not found
     */
    static size_t getStateSize(const char* appId);

private:
    static bool _initialized;
    static Preferences _prefs;

    // NVS namespace for app states
    static constexpr const char* NAMESPACE = "doki_states";

    // Maximum state size (4KB)
    static constexpr size_t MAX_STATE_SIZE = 4096;

    // Helper: Generate NVS key from app ID
    static String _makeKey(const char* appId);
};

} // namespace Doki

#endif // DOKI_STATE_PERSISTENCE_H
