/**
 * @file lvgl_manager.cpp
 * @brief Implementation of LVGL Thread Safety Manager
 */

#include "doki/lvgl_manager.h"

namespace Doki {

// Static member initialization
SemaphoreHandle_t LVGLManager::_mutex = nullptr;
bool LVGLManager::_initialized = false;

bool LVGLManager::init() {
    if (_initialized) {
        Serial.println("[LVGLManager] Already initialized");
        return true;
    }

    Serial.println("[LVGLManager] Initializing LVGL mutex...");

    // Create RECURSIVE mutex (allows same task to acquire multiple times)
    _mutex = xSemaphoreCreateRecursiveMutex();

    if (_mutex == nullptr) {
        Serial.println("[LVGLManager] ✗ Failed to create mutex");
        return false;
    }

    _initialized = true;
    Serial.println("[LVGLManager] ✓ LVGL recursive mutex initialized");

    return true;
}

void LVGLManager::lock() {
    if (!_initialized || _mutex == nullptr) {
        Serial.println("[LVGLManager] Warning: Mutex not initialized, skipping lock");
        return;
    }

    // Wait indefinitely for recursive mutex
    xSemaphoreTakeRecursive(_mutex, portMAX_DELAY);
}

void LVGLManager::unlock() {
    if (!_initialized || _mutex == nullptr) {
        return;
    }

    // Release recursive mutex
    xSemaphoreGiveRecursive(_mutex);
}

bool LVGLManager::tryLock(uint32_t timeoutMs) {
    if (!_initialized || _mutex == nullptr) {
        Serial.println("[LVGLManager] Warning: Mutex not initialized");
        return false;
    }

    // Convert ms to ticks
    TickType_t ticks = pdMS_TO_TICKS(timeoutMs);

    // Try to acquire recursive mutex with timeout
    return xSemaphoreTakeRecursive(_mutex, ticks) == pdTRUE;
}

} // namespace Doki
