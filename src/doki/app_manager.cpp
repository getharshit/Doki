/**
 * @file app_manager.cpp
 * @brief Implementation of App Manager for Doki OS
 */

#include "doki/app_manager.h"
#include "doki/event_system.h"
#include "doki/memory_manager.h"
#include "doki/task_scheduler.h"

namespace Doki {

// ========================================
// Static Member Initialization
// ========================================

std::map<std::string, AppRegistration> AppManager::_registry;
DokiApp* AppManager::_currentApp = nullptr;
std::string AppManager::_currentAppId = "";

// ========================================
// Public Methods
// ========================================

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

bool AppManager::loadApp(const char* appId) {
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
    
    // If same app already running, do nothing
    if (_currentApp && _currentAppId == appId) {
        Serial.printf("[AppManager] App '%s' already running\n", appId);
        return true;
    }
    
    // Unload current app if any
    if (_currentApp) {
        Serial.printf("[AppManager] Unloading current app '%s' before loading '%s'\n",
                      _currentAppId.c_str(), appId);
        unloadCurrentApp();
    }
    
    Serial.println("╔═══════════════════════════════════╗");
    Serial.printf("║ Loading App: %-19s ║\n", reg->name.c_str());
    Serial.println("╚═══════════════════════════════════╝");
    
    // Start memory tracking
    MemoryManager::startTracking(appId);
    
    // Create app instance using factory
    Serial.printf("[AppManager] Creating app instance...\n");
    _currentApp = reg->factory();
    
    if (!_currentApp) {
        Serial.printf("[AppManager] Error: Failed to create app '%s'\n", appId);
        MemoryManager::stopTracking(appId);
        return false;
    }
    
    _currentAppId = appId;
    
    // Publish APP_LOADED event
    EventSystem::publish(EventType::APP_LOADED, "AppManager", (void*)appId);
    
    // Call onCreate
    Serial.printf("[AppManager] Calling onCreate()...\n");
    _currentApp->onCreate();
    _currentApp->_setState(AppState::CREATED);
    
    // Call onStart
    Serial.printf("[AppManager] Calling onStart()...\n");
    _currentApp->onStart();
    _currentApp->_setState(AppState::STARTED);
    _currentApp->_markStarted();
    
    // Publish APP_STARTED event
    EventSystem::publish(EventType::APP_STARTED, "AppManager", (void*)appId);
    
    Serial.printf("[AppManager] ✓ App '%s' loaded successfully\n", reg->name.c_str());
    Serial.printf("[AppManager] Uptime: 0ms\n\n");
    
    return true;
}

bool AppManager::unloadCurrentApp() {
    if (!_currentApp) {
        Serial.println("[AppManager] No app to unload");
        return false;
    }
    
    const char* appId = _currentAppId.c_str();
    
    Serial.println("╔═══════════════════════════════════╗");
    Serial.printf("║ Unloading App: %-18s ║\n", _currentApp->getName());
    Serial.println("╚═══════════════════════════════════╝");
    
    // Get final uptime
    uint32_t uptime = _currentApp->getUptime();
    Serial.printf("[AppManager] App uptime: %lu ms\n", uptime);
    
    // Publish APP_PAUSED event
    EventSystem::publish(EventType::APP_PAUSED, "AppManager", (void*)appId);
    
    // Call onPause
    Serial.printf("[AppManager] Calling onPause()...\n");
    _currentApp->onPause();
    _currentApp->_setState(AppState::PAUSED);
    
    // Call onDestroy
    Serial.printf("[AppManager] Calling onDestroy()...\n");
    _currentApp->onDestroy();
    _currentApp->_setState(AppState::DESTROYED);
    
    // Cleanup
    _cleanupApp(_currentApp, appId);
    
    // Delete app instance
    delete _currentApp;
    _currentApp = nullptr;
    
    // Publish APP_UNLOADED event
    EventSystem::publish(EventType::APP_UNLOADED, "AppManager", (void*)appId);
    
    Serial.printf("[AppManager] ✓ App '%s' unloaded\n\n", appId);
    
    _currentAppId = "";
    
    return true;
}

void AppManager::update() {
    if (_currentApp && _currentApp->isRunning()) {
        _currentApp->onUpdate();
    }
}

DokiApp* AppManager::getCurrentApp() {
    return _currentApp;
}

const char* AppManager::getCurrentAppId() {
    return _currentAppId.c_str();
}

bool AppManager::isAppRegistered(const char* appId) {
    return _registry.find(appId) != _registry.end();
}

bool AppManager::isAppRunning() {
    return _currentApp != nullptr;
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

void AppManager::printAppRegistry() {
    Serial.println("┌─────────────────────────────────────┐");
    Serial.println("│ Registered Apps                     │");
    Serial.println("├─────────────────────────────────────┤");
    
    if (_registry.empty()) {
        Serial.println("│ No apps registered                  │");
    } else {
        for (const auto& pair : _registry) {
            const AppRegistration& reg = pair.second;
            Serial.printf("│ [%s] %s\n", reg.id.c_str(), reg.name.c_str());
            if (!reg.description.empty()) {
                Serial.printf("│   %s\n", reg.description.c_str());
            }
        }
    }
    
    Serial.println("└─────────────────────────────────────┘");
    
    if (_currentApp) {
        Serial.printf("\nCurrently running: %s (uptime: %lu ms)\n", 
                      _currentApp->getName(), _currentApp->getUptime());
    } else {
        Serial.println("\nNo app currently running");
    }
}

uint32_t AppManager::getAppUptime() {
    return _currentApp ? _currentApp->getUptime() : 0;
}

void AppManager::clearRegistry() {
    Serial.printf("[AppManager] Clearing registry (%d apps)\n", _registry.size());
    
    // Unload current app first
    if (_currentApp) {
        unloadCurrentApp();
    }
    
    _registry.clear();
}

// ========================================
// Private Helper Methods
// ========================================

AppRegistration* AppManager::_findRegistration(const char* appId) {
    auto it = _registry.find(appId);
    if (it != _registry.end()) {
        return &(it->second);
    }
    return nullptr;
}

void AppManager::_cleanupApp(DokiApp* app, const char* appId) {
    Serial.println("[AppManager] Cleaning up app resources...");
    
    // Stop all tasks for this app
    int tasksStopped = TaskScheduler::stopAppTasks(appId);
    if (tasksStopped > 0) {
        Serial.printf("[AppManager] Stopped %d task(s)\n", tasksStopped);
    }
    
    // Check for memory leaks
    bool memoryClean = MemoryManager::stopTracking(appId);
    if (!memoryClean) {
        Serial.println("[AppManager] ⚠️  Warning: Memory leaks detected!");
    }
    
    // Clear the screen
    app->clearScreen();
    
    Serial.println("[AppManager] Cleanup complete");
}

} // namespace Doki