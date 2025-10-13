/**
 * @file display_manager.h
 * @brief Display Manager for Doki OS - Multi-display support
 * 
 * Manages multiple ST7789 displays with independent control.
 * Supports up to 3 displays with separate LVGL screens.
 * 
 * Example:
 *   DisplayManager::init(2);  // Initialize 2 displays
 *   DisplayManager::assignApp(0, clockApp);
 *   DisplayManager::assignApp(1, weatherApp);
 */

#ifndef DOKI_DISPLAY_MANAGER_H
#define DOKI_DISPLAY_MANAGER_H

#include <Arduino.h>
#include <lvgl.h>
#include "doki/app_base.h"

namespace Doki {

/**
 * @brief Display configuration
 */
struct DisplayConfig {
    uint8_t id;          // Display ID (0, 1, 2)
    uint8_t cs_pin;      // Chip select pin
    uint8_t dc_pin;      // Data/command pin
    uint8_t rst_pin;     // Reset pin
    uint16_t width;      // Display width
    uint16_t height;     // Display height
    
    DisplayConfig() 
        : id(0), cs_pin(0), dc_pin(0), rst_pin(0)
        , width(240), height(320) {}
};

/**
 * @brief Display information
 */
struct DisplayInfo {
    uint8_t id;                    // Display ID
    DisplayConfig config;          // Hardware configuration
    lv_disp_t* lvgl_display;      // LVGL display object
    DokiApp* currentApp;           // Currently running app
    bool initialized;              // Whether display is initialized
    
    DisplayInfo() 
        : id(0), lvgl_display(nullptr), currentApp(nullptr)
        , initialized(false) {}
};

/**
 * @brief Display Manager - Multi-display controller
 * 
 * Manages multiple displays with independent app control
 */
class DisplayManager {
public:
    /**
     * @brief Initialize display manager
     * 
     * @param displayCount Number of displays (1-3)
     * @return true if initialization succeeded
     * 
     * Must be called before using any displays.
     * 
     * Example:
     *   DisplayManager::init(2);  // 2 displays
     */
    static bool init(uint8_t displayCount);
    
    /**
     * @brief Get display configuration for a display ID
     * 
     * @param displayId Display ID (0, 1, 2)
     * @return Display configuration
     */
    static DisplayConfig getDisplayConfig(uint8_t displayId);
    
    /**
     * @brief Check if a display is initialized
     * 
     * @param displayId Display ID
     * @return true if display is ready
     */
    static bool isDisplayReady(uint8_t displayId);
    
    /**
     * @brief Get number of initialized displays
     * 
     * @return Display count
     */
    static uint8_t getDisplayCount();
    
    /**
     * @brief Assign an app to a display
     * 
     * @param displayId Display ID
     * @param app App to assign (nullptr to clear)
     * @return true if assignment succeeded
     * 
     * Example:
     *   DisplayManager::assignApp(0, clockApp);
     *   DisplayManager::assignApp(1, weatherApp);
     */
    static bool assignApp(uint8_t displayId, DokiApp* app);
    
    /**
     * @brief Get current app on a display
     * 
     * @param displayId Display ID
     * @return Current app (nullptr if none)
     */
    static DokiApp* getCurrentApp(uint8_t displayId);
    
    /**
     * @brief Update all displays
     * 
     * Call in loop() to update LVGL and apps
     */
    static void updateAll();
    
    /**
     * @brief Get LVGL display for a display ID
     * 
     * @param displayId Display ID
     * @return LVGL display object
     */
    static lv_disp_t* getLvglDisplay(uint8_t displayId);
    
    /**
     * @brief Print display status
     */
    static void printStatus();

private:
    static const uint8_t MAX_DISPLAYS = 3;
    static DisplayInfo _displays[MAX_DISPLAYS];
    static uint8_t _displayCount;
    static bool _initialized;
    
    // Initialize a single display
    static bool _initDisplay(uint8_t displayId, const DisplayConfig& config);
    
    // Get predefined configurations
    static DisplayConfig _getDefaultConfig(uint8_t displayId);
};

} // namespace Doki

#endif // DOKI_DISPLAY_MANAGER_H