/**
 * @file js_engine.cpp
 * @brief Implementation of JavaScript Engine for Doki OS
 */

#include "doki/js_engine.h"
#include "doki/filesystem_manager.h"
#include "doki/state_persistence.h"
#include <lvgl.h>

namespace Doki {

// Static member initialization
bool JSEngine::_initialized = false;
String JSEngine::_lastError = "";

bool JSEngine::init() {
    if (_initialized) {
        Serial.println("[JSEngine] Already initialized");
        return true;
    }

#ifdef ENABLE_JAVASCRIPT_SUPPORT
    Serial.println("[JSEngine] Initializing Duktape...");
    _initialized = true;
    Serial.println("[JSEngine] ✓ Duktape initialized");
    return true;
#else
    Serial.println("[JSEngine] JavaScript support not enabled");
    Serial.println("[JSEngine] To enable: Download Duktape from https://duktape.org/");
    Serial.println("[JSEngine] Add duktape.c/h to lib/duktape/ and uncomment ENABLE_JAVASCRIPT_SUPPORT");
    _lastError = "JavaScript support not compiled in";
    return false;
#endif
}

void* JSEngine::createContext() {
#ifdef ENABLE_JAVASCRIPT_SUPPORT
    duk_context* ctx = duk_create_heap_default();
    if (!ctx) {
        _lastError = "Failed to create Duktape heap";
        Serial.println("[JSEngine] Error: Failed to create context");
        return nullptr;
    }

    // Register Doki OS APIs
    registerDokiAPIs(ctx);

    Serial.println("[JSEngine] ✓ Created JS context");
    return ctx;
#else
    _lastError = "JavaScript support not enabled";
    return nullptr;
#endif
}

void JSEngine::destroyContext(void* ctx) {
#ifdef ENABLE_JAVASCRIPT_SUPPORT
    if (ctx) {
        duk_destroy_heap((duk_context*)ctx);
        Serial.println("[JSEngine] Context destroyed");
    }
#endif
}

bool JSEngine::loadScript(void* ctx, const char* filepath) {
#ifdef ENABLE_JAVASCRIPT_SUPPORT
    if (!ctx || !filepath) {
        _lastError = "Invalid context or filepath";
        return false;
    }

    // Read JS file from SPIFFS
    File file = FilesystemManager::openFile(filepath, "r");
    if (!file) {
        _lastError = String("Failed to open: ") + filepath;
        Serial.printf("[JSEngine] Error: %s\n", _lastError.c_str());
        return false;
    }

    String code = file.readString();
    file.close();

    if (code.isEmpty()) {
        _lastError = "Empty JavaScript file";
        Serial.println("[JSEngine] Error: Empty file");
        return false;
    }

    Serial.printf("[JSEngine] Loaded %d bytes from %s\n", code.length(), filepath);

    // Execute the script
    return executeScript(ctx, code.c_str());
#else
    _lastError = "JavaScript support not enabled";
    return false;
#endif
}

bool JSEngine::executeScript(void* ctx, const char* code) {
#ifdef ENABLE_JAVASCRIPT_SUPPORT
    if (!ctx || !code) {
        _lastError = "Invalid context or code";
        return false;
    }

    duk_context* duk_ctx = (duk_context*)ctx;

    // Evaluate the script
    if (duk_peval_string(duk_ctx, code) != 0) {
        _lastError = String("Script error: ") + duk_safe_to_string(duk_ctx, -1);
        Serial.printf("[JSEngine] Error: %s\n", _lastError.c_str());
        duk_pop(duk_ctx);
        return false;
    }

    duk_pop(duk_ctx);  // Pop result
    Serial.println("[JSEngine] ✓ Script executed successfully");
    return true;
#else
    _lastError = "JavaScript support not enabled";
    return false;
#endif
}

bool JSEngine::callFunction(void* ctx, const char* funcName) {
#ifdef ENABLE_JAVASCRIPT_SUPPORT
    if (!ctx || !funcName) {
        _lastError = "Invalid context or function name";
        return false;
    }

    duk_context* duk_ctx = (duk_context*)ctx;

    // Get function from global object
    duk_push_global_object(duk_ctx);
    duk_get_prop_string(duk_ctx, -1, funcName);

    if (!duk_is_function(duk_ctx, -1)) {
        // Function doesn't exist - this is OK for optional lifecycle methods
        duk_pop_2(duk_ctx);
        return true;  // Not an error
    }

    // Call function
    if (duk_pcall(duk_ctx, 0) != 0) {
        _lastError = String("Function error: ") + duk_safe_to_string(duk_ctx, -1);
        Serial.printf("[JSEngine] Error in %s(): %s\n", funcName, _lastError.c_str());
        duk_pop_2(duk_ctx);
        return false;
    }

    duk_pop_2(duk_ctx);  // Pop result and global
    return true;
#else
    _lastError = "JavaScript support not enabled";
    return false;
#endif
}

bool JSEngine::callFunctionWithArgs(void* ctx, const char* funcName, const JsonDocument& args) {
#ifdef ENABLE_JAVASCRIPT_SUPPORT
    if (!ctx || !funcName) {
        _lastError = "Invalid context or function name";
        return false;
    }

    duk_context* duk_ctx = (duk_context*)ctx;

    // Get function
    duk_push_global_object(duk_ctx);
    duk_get_prop_string(duk_ctx, -1, funcName);

    if (!duk_is_function(duk_ctx, -1)) {
        duk_pop_2(duk_ctx);
        return true;  // Not an error if function doesn't exist
    }

    // Convert JSON to JavaScript object
    String jsonStr;
    serializeJson(args, jsonStr);
    duk_push_string(duk_ctx, jsonStr.c_str());
    duk_json_decode(duk_ctx, -1);

    // Call function with argument
    if (duk_pcall(duk_ctx, 1) != 0) {
        _lastError = String("Function error: ") + duk_safe_to_string(duk_ctx, -1);
        Serial.printf("[JSEngine] Error in %s(): %s\n", funcName, _lastError.c_str());
        duk_pop_2(duk_ctx);
        return false;
    }

    duk_pop_2(duk_ctx);
    return true;
#else
    _lastError = "JavaScript support not enabled";
    return false;
#endif
}

void JSEngine::registerDokiAPIs(void* ctx) {
#ifdef ENABLE_JAVASCRIPT_SUPPORT
    duk_context* duk_ctx = (duk_context*)ctx;

    // Register log() function
    duk_push_c_function(duk_ctx, _js_log, 1);
    duk_put_global_string(duk_ctx, "log");

    // Register createLabel() function
    duk_push_c_function(duk_ctx, _js_createLabel, 3);
    duk_put_global_string(duk_ctx, "createLabel");

    // Register createButton() function
    duk_push_c_function(duk_ctx, _js_createButton, 3);
    duk_put_global_string(duk_ctx, "createButton");

    // Register setBackgroundColor() function
    duk_push_c_function(duk_ctx, _js_setBackgroundColor, 1);
    duk_put_global_string(duk_ctx, "setBackgroundColor");

    // Register getDisplayId() function
    duk_push_c_function(duk_ctx, _js_getDisplayId, 0);
    duk_put_global_string(duk_ctx, "getDisplayId");

    // Register saveState() function
    duk_push_c_function(duk_ctx, _js_saveState, 2);
    duk_put_global_string(duk_ctx, "saveState");

    // Register loadState() function
    duk_push_c_function(duk_ctx, _js_loadState, 1);
    duk_put_global_string(duk_ctx, "loadState");

    Serial.println("[JSEngine] ✓ Registered Doki OS APIs");
#endif
}

const char* JSEngine::getLastError() {
    return _lastError.c_str();
}

bool JSEngine::isEnabled() {
#ifdef ENABLE_JAVASCRIPT_SUPPORT
    return true;
#else
    return false;
#endif
}

// ========================================
// Duktape API Binding Functions
// ========================================

#ifdef ENABLE_JAVASCRIPT_SUPPORT

duk_ret_t JSEngine::_js_log(duk_context* ctx) {
    const char* message = duk_to_string(ctx, 0);
    Serial.printf("[JS] %s\n", message);
    return 0;
}

duk_ret_t JSEngine::_js_createLabel(duk_context* ctx) {
    const char* text = duk_to_string(ctx, 0);
    int x = duk_to_int(ctx, 1);
    int y = duk_to_int(ctx, 2);

    lv_obj_t* label = lv_label_create(lv_scr_act());
    lv_label_set_text(label, text);
    lv_obj_set_pos(label, x, y);

    Serial.printf("[JS] Created label: '%s' at (%d, %d)\n", text, x, y);
    return 0;
}

duk_ret_t JSEngine::_js_createButton(duk_context* ctx) {
    const char* text = duk_to_string(ctx, 0);
    int x = duk_to_int(ctx, 1);
    int y = duk_to_int(ctx, 2);

    lv_obj_t* btn = lv_btn_create(lv_scr_act());
    lv_obj_set_pos(btn, x, y);

    lv_obj_t* label = lv_label_create(btn);
    lv_label_set_text(label, text);
    lv_obj_center(label);

    Serial.printf("[JS] Created button: '%s' at (%d, %d)\n", text, x, y);
    return 0;
}

duk_ret_t JSEngine::_js_setBackgroundColor(duk_context* ctx) {
    uint32_t color = duk_to_uint32(ctx, 0);

    lv_obj_t* screen = lv_scr_act();
    lv_obj_set_style_bg_color(screen, lv_color_hex(color), 0);

    Serial.printf("[JS] Set background color: 0x%06X\n", color);
    return 0;
}

duk_ret_t JSEngine::_js_getDisplayId(duk_context* ctx) {
    // This would need to be set per-app context
    // For now, return 0
    duk_push_int(ctx, 0);
    return 1;
}

duk_ret_t JSEngine::_js_saveState(duk_context* ctx) {
    const char* key = duk_to_string(ctx, 0);
    const char* value = duk_to_string(ctx, 1);

    // Save to NVS
    JsonDocument state;
    state[key] = value;
    bool success = StatePersistence::saveState("jsapp", state);

    duk_push_boolean(ctx, success);
    return 1;
}

duk_ret_t JSEngine::_js_loadState(duk_context* ctx) {
    const char* key = duk_to_string(ctx, 0);

    // Load from NVS
    JsonDocument state;
    if (StatePersistence::loadState("jsapp", state)) {
        const char* value = state[key];
        duk_push_string(ctx, value);
        return 1;
    }

    duk_push_null(ctx);
    return 1;
}

#endif // ENABLE_JAVASCRIPT_SUPPORT

} // namespace Doki
