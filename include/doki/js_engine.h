/**
 * @file js_engine.h
 * @brief JavaScript Engine for Doki OS (Duktape-based)
 *
 * Provides JavaScript execution environment for running JS apps.
 * Apps can be loaded from SPIFFS and executed with access to Doki OS APIs.
 *
 * To enable JavaScript support:
 * 1. Download Duktape from https://duktape.org/download.html
 * 2. Extract duktape.c and duktape.h to lib/duktape/
 * 3. Uncomment #define ENABLE_JAVASCRIPT_SUPPORT below
 *
 * Example JS App (myapp.js):
 *   function onCreate() {
 *       log("Hello from JavaScript!");
 *       createLabel("Hello World!", 120, 160);
 *   }
 *   function onUpdate() {
 *       // Update logic
 *   }
 */

#ifndef DOKI_JS_ENGINE_H
#define DOKI_JS_ENGINE_H

#include <Arduino.h>
#include <ArduinoJson.h>

// JavaScript support - ENABLED with proper duk_config.h from Duktape 2.7.0
#define ENABLE_JAVASCRIPT_SUPPORT

// Include Duktape from lib/duktape directory
#ifdef ENABLE_JAVASCRIPT_SUPPORT
    #include "duktape.h"
#endif

namespace Doki {

/**
 * @brief JavaScript execution context
 *
 * Manages a Duktape context with Doki OS API bindings
 */
class JSEngine {
public:
    /**
     * @brief Initialize JavaScript engine
     * @return true if initialized successfully
     */
    static bool init();

    /**
     * @brief Create a new JS context
     * @return Pointer to context, or nullptr on failure
     */
    static void* createContext();

    /**
     * @brief Destroy a JS context
     * @param ctx Context to destroy
     */
    static void destroyContext(void* ctx);

    /**
     * @brief Load and execute JavaScript file
     *
     * @param ctx JS context
     * @param filepath Path to .js file in SPIFFS (e.g., "/apps/myapp.js")
     * @return true if loaded and executed successfully
     */
    static bool loadScript(void* ctx, const char* filepath);

    /**
     * @brief Execute JavaScript code
     *
     * @param ctx JS context
     * @param code JavaScript code string
     * @return true if executed successfully
     */
    static bool executeScript(void* ctx, const char* code);

    /**
     * @brief Call a JavaScript function
     *
     * @param ctx JS context
     * @param funcName Function name (e.g., "onCreate")
     * @return true if function exists and was called successfully
     */
    static bool callFunction(void* ctx, const char* funcName);

    /**
     * @brief Call a JavaScript function with JSON argument
     *
     * @param ctx JS context
     * @param funcName Function name
     * @param args JSON document to pass as argument
     * @return true if successful
     */
    static bool callFunctionWithArgs(void* ctx, const char* funcName, const JsonDocument& args);

    /**
     * @brief Register Doki OS APIs to JS context
     *
     * @param ctx JS context
     *
     * Exposes:
     * - log(message) - Console logging
     *
     * UI Creation:
     * - createLabel(text, x, y) - Create LVGL label (returns ID)
     * - updateLabel(id, newText) - Update label text
     * - setLabelColor(id, hex) - Set label text color
     * - setLabelSize(id, size) - Set label font size (12/14/16/20/24)
     * - createButton(text, x, y) - Create LVGL button (decorative)
     * - setBackgroundColor(hex) - Set screen background
     * - clearScreen() - Remove all objects from screen
     *
     * Drawing:
     * - drawRectangle(x, y, w, h, color) - Draw filled rectangle
     * - drawCircle(x, y, radius, color) - Draw filled circle
     *
     * Advanced Text:
     * - createScrollingLabel(text, x, y, width) - Auto-scrolling text
     * - setTextAlign(id, align) - Set text alignment (0=left, 1=center, 2=right)
     *
     * Screen Info:
     * - getWidth() - Get screen width
     * - getHeight() - Get screen height
     * - getDisplayId() - Get current display ID
     *
     * Text Styling:
     * - setTextColor(objId, hex) - Set text color
     * - setTextSize(objId, size) - Set text font size
     *
     * State:
     * - saveState(key, value) - Save persistent state
     * - loadState(key) - Load persistent state
     *
     * Time:
     * - millis() - Get milliseconds since boot
     * - getTime() - Get real time (returns object with hour, minute, second, day, month, year)
     *
     * HTTP:
     * - httpGet(url) - Fetch data from URL (returns response text or null)
     *
     * Animations:
     * - fadeIn(id, duration) - Fade in animation (ms)
     * - fadeOut(id, duration) - Fade out animation (ms)
     * - moveLabel(id, x, y, duration) - Move with animation (ms)
     * - setOpacity(id, opacity) - Set opacity 0-255
     *
     * Multi-Display:
     * - getDisplayCount() - Get total number of displays
     * - sendToDisplay(displayId, message) - Send message to another display
     * - onMessage(callback) - Register message callback
     *
     * MQTT:
     * - mqttConnect(broker, port, clientId) - Connect to MQTT broker
     * - mqttPublish(topic, message) - Publish message
     * - mqttSubscribe(topic, callback) - Subscribe with callback
     * - mqttDisconnect() - Disconnect from broker
     *
     * WebSocket:
     * - wsConnect(url) - Connect to WebSocket server
     * - wsSend(message) - Send message
     * - wsOnMessage(callback) - Register message callback
     * - wsDisconnect() - Disconnect
     */
    static void registerDokiAPIs(void* ctx);

    /**
     * @brief Set the display ID for a JS context
     * @param ctx JS context
     * @param displayId Display ID (0, 1, etc.)
     */
    static void setDisplayId(void* ctx, uint8_t displayId);

    /**
     * @brief Set display screen pointer for this context
     * @param ctx JS context
     * @param screen LVGL screen pointer (lv_obj_t*)
     */
    static void setDisplayScreen(void* ctx, void* screen);

    /**
     * @brief Get last error message
     * @return Error string
     */
    static const char* getLastError();

    /**
     * @brief Check if JavaScript support is enabled
     * @return true if compiled with ENABLE_JAVASCRIPT_SUPPORT
     */
    static bool isEnabled();

private:
    static bool _initialized;
    static String _lastError;

#ifdef ENABLE_JAVASCRIPT_SUPPORT
    // Duktape API binding functions - Basic
    static duk_ret_t _js_log(duk_context* ctx);

    // UI Creation
    static duk_ret_t _js_createLabel(duk_context* ctx);
    static duk_ret_t _js_updateLabel(duk_context* ctx);
    static duk_ret_t _js_setLabelColor(duk_context* ctx);
    static duk_ret_t _js_setLabelSize(duk_context* ctx);
    static duk_ret_t _js_createButton(duk_context* ctx);
    static duk_ret_t _js_setBackgroundColor(duk_context* ctx);
    static duk_ret_t _js_clearScreen(duk_context* ctx);

    // Drawing
    static duk_ret_t _js_drawRectangle(duk_context* ctx);
    static duk_ret_t _js_drawCircle(duk_context* ctx);

    // Advanced Text
    static duk_ret_t _js_createScrollingLabel(duk_context* ctx);
    static duk_ret_t _js_setTextAlign(duk_context* ctx);

    // Screen Info
    static duk_ret_t _js_getWidth(duk_context* ctx);
    static duk_ret_t _js_getHeight(duk_context* ctx);
    static duk_ret_t _js_getDisplayId(duk_context* ctx);

    // Text Styling
    static duk_ret_t _js_setTextColor(duk_context* ctx);
    static duk_ret_t _js_setTextSize(duk_context* ctx);

    // State Persistence
    static duk_ret_t _js_saveState(duk_context* ctx);
    static duk_ret_t _js_loadState(duk_context* ctx);

    // Time
    static duk_ret_t _js_millis(duk_context* ctx);
    static duk_ret_t _js_getTime(duk_context* ctx);

    // HTTP
    static duk_ret_t _js_httpGet(duk_context* ctx);

    // Animations
    static duk_ret_t _js_fadeIn(duk_context* ctx);
    static duk_ret_t _js_fadeOut(duk_context* ctx);
    static duk_ret_t _js_moveLabel(duk_context* ctx);
    static duk_ret_t _js_setOpacity(duk_context* ctx);

    // Multi-Display
    static duk_ret_t _js_getDisplayCount(duk_context* ctx);
    static duk_ret_t _js_sendToDisplay(duk_context* ctx);

    // MQTT
    static duk_ret_t _js_mqttConnect(duk_context* ctx);
    static duk_ret_t _js_mqttPublish(duk_context* ctx);
    static duk_ret_t _js_mqttSubscribe(duk_context* ctx);
    static duk_ret_t _js_mqttDisconnect(duk_context* ctx);

    // WebSocket
    static duk_ret_t _js_wsConnect(duk_context* ctx);
    static duk_ret_t _js_wsIsConnected(duk_context* ctx);
    static duk_ret_t _js_wsSend(duk_context* ctx);
    static duk_ret_t _js_wsOnMessage(duk_context* ctx);
    static duk_ret_t _js_wsDisconnect(duk_context* ctx);

    // Animation
    static duk_ret_t _js_loadAnimation(duk_context* ctx);
    static duk_ret_t _js_playAnimation(duk_context* ctx);
    static duk_ret_t _js_stopAnimation(duk_context* ctx);
    static duk_ret_t _js_pauseAnimation(duk_context* ctx);
    static duk_ret_t _js_resumeAnimation(duk_context* ctx);
    static duk_ret_t _js_setAnimationPosition(duk_context* ctx);
    static duk_ret_t _js_setAnimationSpeed(duk_context* ctx);
    static duk_ret_t _js_setAnimationOpacity(duk_context* ctx);
    static duk_ret_t _js_unloadAnimation(duk_context* ctx);
    static duk_ret_t _js_updateAnimations(duk_context* ctx);
#endif
};

} // namespace Doki

#endif // DOKI_JS_ENGINE_H
