/**
 * @file js_app.h
 * @brief JavaScript App Wrapper for Doki OS
 *
 * Allows JavaScript files to be loaded and executed as Doki OS apps.
 *
 * Example: Create /data/apps/hello.js:
 *   function onCreate() {
 *       log("Hello from JavaScript!");
 *       createLabel("Hello JS!", 60, 140);
 *       setBackgroundColor(0x001122);
 *   }
 *
 *   function onUpdate() {
 *       // Update logic here
 *   }
 *
 *   function onDestroy() {
 *       log("Goodbye!");
 *   }
 *
 * Then register it:
 *   AppManager::registerApp("hello_js", "Hello JS",
 *       []() { return new JSApp("hello_js", "Hello JS", "/apps/hello.js"); });
 */

#ifndef DOKI_JS_APP_H
#define DOKI_JS_APP_H

#include "doki/app_base.h"
#include "doki/js_engine.h"
#include <ArduinoJson.h>

namespace Doki {

/**
 * @brief JavaScript App Wrapper
 *
 * Wraps a JavaScript file as a Doki OS app with full lifecycle support
 */
class JSApp : public DokiApp {
public:
    /**
     * @brief Constructor
     *
     * @param id App unique ID (e.g., "hello_js")
     * @param name App display name (e.g., "Hello JS")
     * @param scriptPath Path to JavaScript file in SPIFFS (e.g., "/apps/hello.js")
     */
    JSApp(const char* id, const char* name, const char* scriptPath);

    /**
     * @brief Destructor
     */
    virtual ~JSApp();

    // Lifecycle methods
    void onCreate() override;
    void onStart() override;
    void onUpdate() override;
    void onPause() override;
    void onDestroy() override;

    // State persistence
    void onSaveState(JsonDocument& state) override;
    void onRestoreState(const JsonDocument& state) override;

protected:
    String _scriptPath;          ///< Path to JS file
    void* _jsContext;            ///< Duktape context
    bool _scriptLoaded;          ///< Script loaded successfully
    uint32_t _lastUpdate;        ///< Last update time (for throttling)
    static const uint32_t UPDATE_INTERVAL = 100;  ///< Update every 100ms

    /**
     * @brief Show error message on screen
     * @param message Error message
     */
    void _showError(const char* message);
};

} // namespace Doki

#endif // DOKI_JS_APP_H
