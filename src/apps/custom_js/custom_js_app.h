/**
 * @file custom_js_app.h
 * @brief Custom JavaScript App - User-programmable via web interface
 *
 * Allows users to write JavaScript code through the web dashboard
 * and execute it on the display. Each display has its own custom JS file.
 *
 * Files are stored in SPIFFS:
 * - Display 0: /js/custom_disp0.js
 * - Display 1: /js/custom_disp1.js
 *
 * Example custom code:
 *   function onCreate() {
 *       log("My custom app!");
 *       setBackgroundColor(0x1a1a2e);
 *       createLabel("Hello World", 70, 140);
 *   }
 */

#ifndef CUSTOM_JS_APP_H
#define CUSTOM_JS_APP_H

#include "doki/js_app.h"

/**
 * @brief Custom JavaScript App
 *
 * Loads and executes user-provided JavaScript code from SPIFFS.
 * Includes watchdog protection and memory monitoring.
 */
class CustomJSApp : public Doki::JSApp {
public:
    /**
     * @brief Constructor
     *
     * Creates a custom JS app that loads code from /js/custom_disp{displayId}.js
     * The display ID is determined automatically when the app is loaded.
     */
    CustomJSApp();

    /**
     * @brief Destructor
     */
    virtual ~CustomJSApp();

    // Override lifecycle to add watchdog protection
    void onCreate() override;
    void onUpdate() override;

    /**
     * @brief Check if custom JS file exists for this app
     * @return true if the JS file exists in SPIFFS
     */
    bool hasCustomCode();

private:
    uint32_t _lastWatchdogReset;     ///< Last time watchdog was reset
    uint32_t _creationStartTime;     ///< onCreate start time
    static const uint32_t WATCHDOG_TIMEOUT = 5000;  ///< 5 second timeout

    /**
     * @brief Reset watchdog timer
     */
    void _resetWatchdog();

    /**
     * @brief Check if watchdog has timed out
     * @return true if execution has exceeded timeout
     */
    bool _watchdogTimeout();
};

#endif // CUSTOM_JS_APP_H
