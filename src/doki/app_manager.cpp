/**
 * @file app_manager.cpp
 * @brief Implementation of App Manager for Doki OS
 */

#include "doki/app_manager.h"
#include "doki/event_system.h"
#include "doki/memory_manager.h"
#include "doki/task_scheduler.h"
#include "doki/state_persistence.h"
#include "doki/lvgl_manager.h"
#include <ArduinoJson.h>

namespace Doki {

// ========================================
// Static Member Initialization
// ========================================

bool AppManager::_initialized = false;
uint8_t AppManager::_numDisplays = 0;
std::vector<DisplayState> AppManager::_displays;
std::map<std::string, AppRegistration> AppManager::_registry;

// ========================================
// Public Methods
// ========================================

bool AppManager::init(uint8_t numDisplays, lv_disp_t** displays) {
    if (_initialized) {
        Serial.println("[AppManager] Warning: Already initialized");
        return true;
    }

    if (numDisplays == 0 || numDisplays > 3) {
        Serial.println("[AppManager] Error: Invalid number of displays (must be 1-3)");
        return false;
    }

    if (!displays) {
        Serial.println("[AppManager] Error: Display array is null");
        return false;
    }

    Serial.printf("[AppManager] Initializing with %d display(s)...\n", numDisplays);

    _numDisplays = numDisplays;
    _displays.clear();

    // Initialize display states
    for (uint8_t i = 0; i < numDisplays; i++) {
        if (!displays[i]) {
            Serial.printf("[AppManager] Error: Display %d is null\n", i);
            return false;
        }
        _displays.push_back(DisplayState(i, displays[i]));
        Serial.printf("[AppManager] Display %d initialized\n", i);
    }

    _initialized = true;
    Serial.println("[AppManager] ✓ Initialization complete");

    return true;
}

bool AppManager::registerApp(const char* id,
                             const char* name,
                             AppFactory factory,
                             const char* description) {
    if (!id || !name || !factory) {
        Serial.println("[AppManager] Error: Invalid registration parameters");
        return false;
    }
    
    // Check if already registered
    if (_registry.find(id) != _registry.end()) {
        Serial.printf("[AppManager] Warning: App '%s' already registered\n", id);
        return false;
    }
    
    // Create registration
    AppRegistration reg(id, name, factory, description);
    _registry[id] = reg;
    
    Serial.printf("[AppManager] Registered app: '%s' (%s)\n", name, id);
    
    return true;
}

bool AppManager::loadApp(uint8_t displayId, const char* appId) {
    if (!_initialized) {
        Serial.println("[AppManager] Error: Not initialized. Call init() first");
        return false;
    }

    if (!_isValidDisplay(displayId)) {
        Serial.printf("[AppManager] Error: Invalid display ID %d\n", displayId);
        return false;
    }

    if (!appId) {
        Serial.println("[AppManager] Error: Invalid app ID");
        return false;
    }

    // Check if app is registered
    AppRegistration* reg = _findRegistration(appId);
    if (!reg) {
        Serial.printf("[AppManager] Error: App '%s' not registered\n", appId);
        return false;
    }

    DisplayState& display = _displays[displayId];

    // If same app already running on this display, do nothing
    if (display.currentApp && display.currentAppId == appId) {
        Serial.printf("[AppManager] App '%s' already running on display %d\n", appId, displayId);
        return true;
    }

    // Unload current app if any
    if (display.currentApp) {
        Serial.printf("[AppManager] Unloading '%s' from display %d before loading '%s'\n",
                      display.currentAppId.c_str(), displayId, appId);
        unloadApp(displayId);
    }

    Serial.println("╔═══════════════════════════════════╗");
    Serial.printf("║ Display %d: Loading %-14s ║\n", displayId, reg->name.c_str());
    Serial.println("╚═══════════════════════════════════╝");

    // Start memory tracking
    char trackingId[64];
    snprintf(trackingId, sizeof(trackingId), "disp%d_%s", displayId, appId);
    MemoryManager::startTracking(trackingId);

    // Create app instance using factory
    Serial.printf("[AppManager] Creating app instance...\n");
    display.currentApp = reg->factory();

    if (!display.currentApp) {
        Serial.printf("[AppManager] Error: Failed to create app '%s'\n", appId);
        MemoryManager::stopTracking(trackingId);
        return false;
    }

    display.currentAppId = appId;

    // Set the app's display
    display.currentApp->setDisplay(display.lvglDisplay);

    // CRITICAL: Acquire LVGL mutex before any LVGL operations
    LVGLManager::lock();

    // CRITICAL: Set LVGL default display BEFORE onCreate()
    // This ensures all UI elements are created on the correct display
    lv_disp_set_default(display.lvglDisplay);

    // Clear the screen to ensure clean slate (removes any remnants from previous app)
    Serial.printf("[AppManager] Clearing screen...\n");
    lv_obj_t* screen = lv_disp_get_scr_act(display.lvglDisplay);
    if (screen) {
        lv_obj_clean(screen);
        lv_obj_set_style_bg_color(screen, lv_color_hex(0x000000), 0); // Black background
        Serial.printf("[AppManager] ✓ Screen cleared\n");
    }

    // Publish APP_LOADED event
    EventSystem::publish(EventType::APP_LOADED, "AppManager", (void*)appId);

    // Call onCreate (may create LVGL objects)
    Serial.printf("[AppManager] Calling onCreate()...\n");
    display.currentApp->onCreate();
    display.currentApp->_setState(AppState::CREATED);

    // Release LVGL mutex after onCreate
    LVGLManager::unlock();

    // Restore state if it exists
    if (StatePersistence::hasState(appId)) {
        Serial.printf("[AppManager] Restoring saved state...\n");
        JsonDocument state;
        if (StatePersistence::loadState(appId, state)) {
            const JsonDocument& constState = state;
            display.currentApp->onRestoreState(constState);
            Serial.printf("[AppManager] ✓ State restored\n");
        }
    }

    // Call onStart (may modify LVGL objects)
    LVGLManager::lock();
    Serial.printf("[AppManager] Calling onStart()...\n");
    display.currentApp->onStart();
    display.currentApp->_setState(AppState::STARTED);
    display.currentApp->_markStarted();
    LVGLManager::unlock();

    // Publish APP_STARTED event
    EventSystem::publish(EventType::APP_STARTED, "AppManager", (void*)appId);

    Serial.printf("[AppManager] ✓ App '%s' loaded on display %d\n", reg->name.c_str(), displayId);
    Serial.printf("[AppManager] Uptime: 0ms\n\n");

    return true;
}

bool AppManager::unloadApp(uint8_t displayId) {
    if (!_initialized) {
        Serial.println("[AppManager] Error: Not initialized");
        return false;
    }

    if (!_isValidDisplay(displayId)) {
        Serial.printf("[AppManager] Error: Invalid display ID %d\n", displayId);
        return false;
    }

    DisplayState& display = _displays[displayId];

    if (!display.currentApp) {
        Serial.printf("[AppManager] Display %d: No app to unload\n", displayId);
        return false;
    }

    const char* appId = display.currentAppId.c_str();

    Serial.println("╔═══════════════════════════════════╗");
    Serial.printf("║ Display %d: Unloading %-12s ║\n", displayId, display.currentApp->getName());
    Serial.println("╚═══════════════════════════════════╝");

    // Get final uptime
    uint32_t uptime = display.currentApp->getUptime();
    Serial.printf("[AppManager] App uptime: %lu ms\n", uptime);

    // Publish APP_PAUSED event
    EventSystem::publish(EventType::APP_PAUSED, "AppManager", (void*)appId);

    // Call onPause
    Serial.printf("[AppManager] Calling onPause()...\n");
    display.currentApp->onPause();
    display.currentApp->_setState(AppState::PAUSED);

    // Save state before destroying
    Serial.printf("[AppManager] Saving app state...\n");
    JsonDocument state;
    display.currentApp->onSaveState(state);
    if (state.size() > 0) {
        if (StatePersistence::saveState(appId, state)) {
            Serial.printf("[AppManager] ✓ State saved\n");
        } else {
            Serial.printf("[AppManager] ⚠️  Failed to save state\n");
        }
    } else {
        Serial.printf("[AppManager] No state to save\n");
    }

    // Call onDestroy
    Serial.printf("[AppManager] Calling onDestroy()...\n");
    display.currentApp->onDestroy();
    display.currentApp->_setState(AppState::DESTROYED);

    // Delete app instance
    delete display.currentApp;
    display.currentApp = nullptr;

    // Cleanup resources (tasks, memory tracking) - AFTER deletion
    _cleanupApp(displayId, appId);

    // Publish APP_UNLOADED event
    EventSystem::publish(EventType::APP_UNLOADED, "AppManager", (void*)appId);

    Serial.printf("[AppManager] ✓ Display %d: App '%s' unloaded\n\n", displayId, appId);

    display.currentAppId = "";

    return true;
}

void AppManager::update() {
    if (!_initialized) return;

    // Acquire LVGL mutex before updating apps (they may modify LVGL)
    LVGLManager::lock();

    // Update all running apps across all displays
    for (auto& display : _displays) {
        if (display.currentApp && display.currentApp->isRunning()) {
            display.currentApp->onUpdate();
        }
    }

    // Release mutex
    LVGLManager::unlock();
}

DokiApp* AppManager::getApp(uint8_t displayId) {
    if (!_isValidDisplay(displayId)) return nullptr;
    return _displays[displayId].currentApp;
}

const char* AppManager::getAppId(uint8_t displayId) {
    if (!_isValidDisplay(displayId)) return "";
    return _displays[displayId].currentAppId.c_str();
}

bool AppManager::isAppRegistered(const char* appId) {
    return _registry.find(appId) != _registry.end();
}

bool AppManager::isAppRunning(uint8_t displayId) {
    if (!_isValidDisplay(displayId)) return false;
    return _displays[displayId].currentApp != nullptr;
}

std::vector<AppRegistration> AppManager::getRegisteredApps() {
    std::vector<AppRegistration> apps;
    for (const auto& pair : _registry) {
        apps.push_back(pair.second);
    }
    return apps;
}

AppRegistration AppManager::getAppInfo(const char* appId) {
    AppRegistration* reg = _findRegistration(appId);
    if (reg) {
        return *reg;
    }
    return AppRegistration();
}

void AppManager::printStatus() {
    Serial.println("╔═════════════════════════════════════╗");
    Serial.println("║ Doki OS - AppManager Status         ║");
    Serial.println("╠═════════════════════════════════════╣");

    // Display status
    Serial.printf("║ Displays: %d                          ║\n", _numDisplays);
    Serial.println("╟─────────────────────────────────────╢");

    for (uint8_t i = 0; i < _numDisplays; i++) {
        const DisplayState& display = _displays[i];
        if (display.currentApp) {
            Serial.printf("║ Display %d: %-23s ║\n", i, display.currentApp->getName());
            Serial.printf("║   App ID: %-25s ║\n", display.currentAppId.c_str());
            Serial.printf("║   Uptime: %-25lu ms ║\n", display.currentApp->getUptime());
        } else {
            Serial.printf("║ Display %d: (no app running)        ║\n", i);
        }
    }

    Serial.println("╠═════════════════════════════════════╣");
    Serial.println("║ Registered Apps                     ║");
    Serial.println("╟─────────────────────────────────────╢");

    if (_registry.empty()) {
        Serial.println("║ No apps registered                  ║");
    } else {
        for (const auto& pair : _registry) {
            const AppRegistration& reg = pair.second;
            Serial.printf("║ [%-6s] %-26s ║\n", reg.id.c_str(), reg.name.c_str());
        }
    }

    Serial.println("╚═════════════════════════════════════╝");
}

uint32_t AppManager::getAppUptime(uint8_t displayId) {
    if (!_isValidDisplay(displayId)) return 0;
    const DisplayState& display = _displays[displayId];
    return display.currentApp ? display.currentApp->getUptime() : 0;
}

uint8_t AppManager::getNumDisplays() {
    return _numDisplays;
}

uint8_t AppManager::getDisplayIdForApp(DokiApp* app) {
    if (!app) return 255;

    // Search for the app in display states
    for (uint8_t i = 0; i < _numDisplays; i++) {
        if (_displays[i].currentApp == app) {
            return i;
        }
    }

    return 255; // Not found
}

void AppManager::clearRegistry() {
    Serial.printf("[AppManager] Clearing registry (%d apps)\n", _registry.size());

    // Unload all apps from all displays
    for (uint8_t i = 0; i < _numDisplays; i++) {
        if (_displays[i].currentApp) {
            unloadApp(i);
        }
    }

    _registry.clear();
}

// ========================================
// Private Helper Methods
// ========================================

bool AppManager::_isValidDisplay(uint8_t displayId) {
    return displayId < _numDisplays;
}

AppRegistration* AppManager::_findRegistration(const char* appId) {
    auto it = _registry.find(appId);
    if (it != _registry.end()) {
        return &(it->second);
    }
    return nullptr;
}

void AppManager::_cleanupApp(uint8_t displayId, const char* appId) {
    Serial.println("[AppManager] Cleaning up app resources...");

    // Create tracking ID
    char trackingId[64];
    snprintf(trackingId, sizeof(trackingId), "disp%d_%s", displayId, appId);

    // Stop all tasks for this app
    int tasksStopped = TaskScheduler::stopAppTasks(trackingId);
    if (tasksStopped > 0) {
        Serial.printf("[AppManager] Stopped %d task(s)\n", tasksStopped);
    }

    // Check for memory leaks
    bool memoryClean = MemoryManager::stopTracking(trackingId);
    if (!memoryClean) {
        Serial.println("[AppManager] ⚠️  Warning: Memory leaks detected!");
    }

    Serial.println("[AppManager] Cleanup complete");
}

} // namespace Doki