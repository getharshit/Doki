/**
 * @file app_manager.h
 * @brief App Manager for Doki OS - Central controller for app lifecycle
 * 
 * The App Manager is responsible for:
 * - Registering apps (app registry)
 * - Loading and unloading apps
 * - Managing app lifecycle (onCreate → onStart → onUpdate → onDestroy)
 * - Coordinating with Memory Manager, Task Scheduler, and Event System
 * - Providing simple API for app switching
 * 
 * Example:
 *   // Register apps
 *   AppManager::registerApp("clock", []() { return new ClockApp(); });
 *   AppManager::registerApp("weather", []() { return new WeatherApp(); });
 *   
 *   // Load an app
 *   AppManager::loadApp("clock");
 *   
 *   // In loop
 *   AppManager::update();
 *   
 *   // Switch to another app
 *   AppManager::loadApp("weather");  // Auto-unloads clock, loads weather
 */

#ifndef DOKI_APP_MANAGER_H
#define DOKI_APP_MANAGER_H

#include <Arduino.h>
#include <map>
#include <vector>
#include <string>
#include <functional>
#include "doki/app_base.h"

namespace Doki {

/**
 * @brief App factory function type
 * 
 * Factory functions create new instances of apps
 */
using AppFactory = std::function<DokiApp*()>;

/**
 * @brief App registration information
 */
struct AppRegistration {
    std::string id;              // App unique ID
    std::string name;            // App display name
    AppFactory factory;          // Factory function to create app
    std::string description;     // Optional description
    
    AppRegistration() {}
    AppRegistration(const std::string& i, const std::string& n, 
                   AppFactory f, const std::string& desc = "")
        : id(i), name(n), factory(f), description(desc) {}
};

/**
 * @brief Display state - tracks what app is running on each display
 */
struct DisplayState {
    uint8_t displayId;           // Display hardware ID (0, 1, 2)
    DokiApp* currentApp;         // Currently running app
    std::string currentAppId;    // Current app ID
    lv_disp_t* lvglDisplay;      // LVGL display handle

    DisplayState() : displayId(0), currentApp(nullptr), currentAppId(""), lvglDisplay(nullptr) {}
    DisplayState(uint8_t id, lv_disp_t* disp)
        : displayId(id), currentApp(nullptr), currentAppId(""), lvglDisplay(disp) {}
};

/**
 * @brief App Manager - Central controller for Doki OS apps
 *
 * Singleton class that manages the entire app lifecycle across multiple displays
 */
class AppManager {
public:
    /**
     * @brief Initialize AppManager with display configuration
     *
     * @param numDisplays Number of displays (1-3)
     * @param displays Array of LVGL display pointers
     * @return true if initialized successfully
     *
     * Must be called once in setup() before any other AppManager operations.
     *
     * Example:
     *   lv_disp_t* displays[2] = {disp0, disp1};
     *   AppManager::init(2, displays);
     */
    static bool init(uint8_t numDisplays, lv_disp_t** displays);

    /**
     * @brief Register an app
     *
     * @param id App unique identifier (e.g., "clock", "weather")
     * @param name App display name (e.g., "Clock", "Weather")
     * @param factory Factory function that creates the app
     * @param description Optional description
     * @return true if registered successfully
     *
     * Must be called before loadApp(). Typically done in setup().
     *
     * Example:
     *   AppManager::registerApp("clock", "Clock App",
     *       []() { return new ClockApp(); },
     *       "Displays current time");
     */
    static bool registerApp(const char* id,
                           const char* name,
                           AppFactory factory,
                           const char* description = "");

    /**
     * @brief Load an app on a specific display
     *
     * @param displayId Display ID (0, 1, 2)
     * @param appId App ID to load
     * @return true if app loaded successfully
     *
     * If another app is running on this display, it will be unloaded first.
     * Calls: factory() → onCreate() → onStart()
     *
     * Example:
     *   AppManager::loadApp(0, "clock");  // Load clock on display 0
     *   AppManager::loadApp(1, "weather"); // Load weather on display 1
     */
    static bool loadApp(uint8_t displayId, const char* appId);

    /**
     * @brief Unload app from a specific display
     *
     * @param displayId Display ID (0, 1, 2)
     * @return true if app unloaded successfully
     *
     * Calls: onPause() → onDestroy()
     * Also stops all app tasks and checks for memory leaks
     */
    static bool unloadApp(uint8_t displayId);

    /**
     * @brief Update all running apps
     *
     * Call this in loop() to update all running apps across all displays.
     * Calls each app's onUpdate() method.
     *
     * Example:
     *   void loop() {
     *       AppManager::update();
     *   }
     */
    static void update();
    
    /**
     * @brief Get app running on a specific display
     *
     * @param displayId Display ID (0, 1, 2)
     * @return Pointer to app (nullptr if none)
     */
    static DokiApp* getApp(uint8_t displayId);

    /**
     * @brief Get app ID running on a specific display
     *
     * @param displayId Display ID (0, 1, 2)
     * @return App ID (empty string if none)
     */
    static const char* getAppId(uint8_t displayId);

    /**
     * @brief Check if an app is registered
     *
     * @param appId App ID to check
     * @return true if app is registered
     */
    static bool isAppRegistered(const char* appId);

    /**
     * @brief Check if any app is running on a display
     *
     * @param displayId Display ID (0, 1, 2)
     * @return true if app is running on this display
     */
    static bool isAppRunning(uint8_t displayId);

    /**
     * @brief Get list of all registered apps
     *
     * @return Vector of app registrations
     */
    static std::vector<AppRegistration> getRegisteredApps();

    /**
     * @brief Get app registration info
     *
     * @param appId App ID
     * @return App registration (empty if not found)
     */
    static AppRegistration getAppInfo(const char* appId);

    /**
     * @brief Print app registry and display status
     *
     * Lists all registered apps and what's running on each display
     */
    static void printStatus();

    /**
     * @brief Get app uptime on a specific display
     *
     * @param displayId Display ID (0, 1, 2)
     * @return App uptime in milliseconds (0 if no app running)
     */
    static uint32_t getAppUptime(uint8_t displayId);

    /**
     * @brief Get number of displays
     *
     * @return Number of configured displays
     */
    static uint8_t getNumDisplays();

    /**
     * @brief Get the display ID for a given app instance
     *
     * @param app Pointer to the app
     * @return Display ID (0, 1, 2) or 255 if not found
     */
    static uint8_t getDisplayIdForApp(DokiApp* app);

    /**
     * @brief Unregister all apps (for testing/cleanup)
     */
    static void clearRegistry();

private:
    // Initialization flag
    static bool _initialized;

    // Number of displays
    static uint8_t _numDisplays;

    // Display states (one per display)
    static std::vector<DisplayState> _displays;

    // App registry (maps app ID to registration info)
    static std::map<std::string, AppRegistration> _registry;

    // Helper: Find app registration
    static AppRegistration* _findRegistration(const char* appId);

    // Helper: Perform full app cleanup (after app deletion)
    static void _cleanupApp(uint8_t displayId, const char* appId);

    // Helper: Validate display ID
    static bool _isValidDisplay(uint8_t displayId);
};

} // namespace Doki

#endif // DOKI_APP_MANAGER_H