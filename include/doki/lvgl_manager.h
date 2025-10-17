/**
 * @file lvgl_manager.h
 * @brief LVGL Thread Safety Manager for Doki OS
 *
 * Provides mutex-based locking to prevent race conditions when accessing
 * LVGL from multiple cores (e.g., HTTP server on Core 0, rendering on Core 1).
 *
 * Usage:
 *   LVGLManager::lock();
 *   lv_obj_clean(screen); // Safe LVGL operation
 *   LVGLManager::unlock();
 */

#ifndef DOKI_LVGL_MANAGER_H
#define DOKI_LVGL_MANAGER_H

#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>

namespace Doki {

/**
 * @brief LVGL Thread Safety Manager
 *
 * Provides mutex-based locking for LVGL operations to prevent
 * race conditions between multiple cores.
 */
class LVGLManager {
public:
    /**
     * @brief Initialize LVGL mutex
     * @return true if initialized successfully
     */
    static bool init();

    /**
     * @brief Acquire LVGL mutex (blocking)
     *
     * Must be called before ANY LVGL operation (lv_*).
     * Always pair with unlock().
     */
    static void lock();

    /**
     * @brief Release LVGL mutex
     *
     * Must be called after LVGL operations.
     * Always pair with lock().
     */
    static void unlock();

    /**
     * @brief Try to acquire LVGL mutex (non-blocking)
     * @param timeoutMs Timeout in milliseconds
     * @return true if mutex acquired, false if timeout
     */
    static bool tryLock(uint32_t timeoutMs = 100);

private:
    static SemaphoreHandle_t _mutex;
    static bool _initialized;
};

} // namespace Doki

#endif // DOKI_LVGL_MANAGER_H
