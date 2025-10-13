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
 * @brief App Manager - Central controller for Doki OS apps
 * 
 * Singleton class that manages the entire app lifecycle
 */
class AppManager {
public:
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
     * @brief Load an app
     * 
     * @param appId App ID to load
     * @return true if app loaded successfully
     * 
     * If another app is running, it will be unloaded first.
     * Calls: factory() → onCreate() → onStart()
     * 
     * Example:
     *   AppManager::loadApp("clock");
     */
    static bool loadApp(const char* appId);
    
    /**
     * @brief Unload current app
     * 
     * @return true if app unloaded successfully
     * 
     * Calls: onPause() → onDestroy()
     * Also stops all app tasks and checks for memory leaks
     */
    static bool unloadCurrentApp();
    
    /**
     * @brief Update current app
     * 
     * Call this in loop() to update the running app.
     * Calls the app's onUpdate() method.
     * 
     * Example:
     *   void loop() {
     *       AppManager::update();
     *   }
     */
    static void update();
    
    /**
     * @brief Get current running app
     * 
     * @return Pointer to current app (nullptr if none)
     */
    static DokiApp* getCurrentApp();
    
    /**
     * @brief Get current app ID
     * 
     * @return Current app ID (empty string if none)
     */
    static const char* getCurrentAppId();
    
    /**
     * @brief Check if an app is registered
     * 
     * @param appId App ID to check
     * @return true if app is registered
     */
    static bool isAppRegistered(const char* appId);
    
    /**
     * @brief Check if an app is currently running
     * 
     * @return true if any app is running
     */
    static bool isAppRunning();
    
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
     * @brief Print app registry
     * 
     * Lists all registered apps with their info
     */
    static void printAppRegistry();
    
    /**
     * @brief Get app uptime
     * 
     * @return Current app uptime in milliseconds (0 if no app running)
     */
    static uint32_t getAppUptime();
    
    /**
     * @brief Unregister all apps (for testing/cleanup)
     */
    static void clearRegistry();

private:
    // App registry (maps app ID to registration info)
    static std::map<std::string, AppRegistration> _registry;
    
    // Current running app
    static DokiApp* _currentApp;
    static std::string _currentAppId;
    
    // Helper: Find app registration
    static AppRegistration* _findRegistration(const char* appId);
    
    // Helper: Perform full app cleanup
    static void _cleanupApp(DokiApp* app, const char* appId);
};

} // namespace Doki

#endif // DOKI_APP_MANAGER_H