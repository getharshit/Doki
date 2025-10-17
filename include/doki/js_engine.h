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

// Uncomment when Duktape is added to lib/duktape/
// #define ENABLE_JAVASCRIPT_SUPPORT

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
     * - createLabel(text, x, y) - Create LVGL label
     * - createButton(text, x, y) - Create LVGL button
     * - setBackgroundColor(hex) - Set screen background
     * - getDisplayId() - Get current display ID
     * - saveState(key, value) - Save persistent state
     * - loadState(key) - Load persistent state
     */
    static void registerDokiAPIs(void* ctx);

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
    // Duktape API binding functions
    static duk_ret_t _js_log(duk_context* ctx);
    static duk_ret_t _js_createLabel(duk_context* ctx);
    static duk_ret_t _js_createButton(duk_context* ctx);
    static duk_ret_t _js_setBackgroundColor(duk_context* ctx);
    static duk_ret_t _js_getDisplayId(duk_context* ctx);
    static duk_ret_t _js_saveState(duk_context* ctx);
    static duk_ret_t _js_loadState(duk_context* ctx);
#endif
};

} // namespace Doki

#endif // DOKI_JS_ENGINE_H
