/**
 * @file js_engine.cpp
 * @brief Implementation of JavaScript Engine for Doki OS
 */

#include "doki/js_engine.h"
#include "doki/filesystem_manager.h"
#include "doki/state_persistence.h"
#include "doki/app_manager.h"
#include <lvgl.h>
#include <HTTPClient.h>
#include <PubSubClient.h>
#include <NTPClient.h>
#include <WiFiUdp.h>

// WebSocket support - enabled for diagnostic testing
#define ENABLE_WEBSOCKET_SUPPORT
#ifdef ENABLE_WEBSOCKET_SUPPORT
    #include <ArduinoWebsockets.h>
#endif

namespace Doki {

// Static member initialization
bool JSEngine::_initialized = false;
String JSEngine::_lastError = "";

// Global NTP Client for JavaScript time functions
// Timezone offset: Asia/Kolkata (UTC+5:30) = 19800 seconds
static WiFiUDP ntpUDP;
static NTPClient* globalTimeClient = nullptr;
static bool ntpInitialized = false;
static TaskHandle_t ntpSyncTaskHandle = nullptr;
static volatile bool ntpSynced = false;

// Background task for NTP sync (prevents blocking main thread)
static void ntpSyncTask(void* parameter) {
    Serial.println("[JSEngine NTP] Background sync task started");

    // Wait a bit for WiFi to stabilize
    vTaskDelay(pdMS_TO_TICKS(2000));

    // Perform initial sync (this will block, but only in background task)
    Serial.println("[JSEngine NTP] Performing initial sync...");
    if (globalTimeClient && globalTimeClient->forceUpdate()) {
        ntpSynced = true;
        Serial.println("[JSEngine NTP] ✓ Time synced successfully");
    } else {
        Serial.println("[JSEngine NTP] ✗ Initial sync failed, will retry");
    }

    // Keep updating every 60 seconds
    while (true) {
        vTaskDelay(pdMS_TO_TICKS(60000));  // Wait 60 seconds

        if (globalTimeClient && globalTimeClient->update()) {
            if (!ntpSynced) {
                ntpSynced = true;
                Serial.println("[JSEngine NTP] ✓ Time synced successfully");
            }
        }
    }
}

// Initialize NTP client in background (non-blocking)
static void initNTPClient() {
    if (ntpInitialized) return;

    Serial.println("[JSEngine] Initializing NTP client (non-blocking)...");
    globalTimeClient = new NTPClient(ntpUDP, "pool.ntp.org", 19800, 60000);
    globalTimeClient->begin();

    // Start background sync task on Core 0
    xTaskCreatePinnedToCore(
        ntpSyncTask,
        "NTP_Sync",
        4096,
        nullptr,
        1,  // Low priority
        &ntpSyncTaskHandle,
        0   // Core 0
    );

    ntpInitialized = true;
    Serial.println("[JSEngine] ✓ NTP client initialized (syncing in background)");
}

bool JSEngine::init() {
    if (_initialized) {
        Serial.println("[JSEngine] Already initialized");
        return true;
    }

#ifdef ENABLE_JAVASCRIPT_SUPPORT
    Serial.println("[JSEngine] Initializing Duktape...");
    _initialized = true;
    Serial.println("[JSEngine] ✓ Duktape initialized");
    Serial.println("[JSEngine] Note: NTP will be initialized after WiFi connection");
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
    uint8_t* data = nullptr;
    size_t size = 0;

    if (!FilesystemManager::readFile(filepath, &data, size)) {
        _lastError = String("Failed to open: ") + filepath;
        Serial.printf("[JSEngine] Error: %s\n", _lastError.c_str());
        return false;
    }

    if (size == 0 || !data) {
        _lastError = "Empty JavaScript file";
        Serial.println("[JSEngine] Error: Empty file");
        if (data) delete[] data;
        return false;
    }

    // Allocate buffer with space for null terminator
    char* code = new char[size + 1];
    if (!code) {
        _lastError = "Out of memory";
        delete[] data;
        return false;
    }

    // Copy data and add null terminator
    memcpy(code, data, size);
    code[size] = '\0';
    delete[] data;

    Serial.printf("[JSEngine] Loaded %d bytes from %s\n", size, filepath);
    Serial.printf("[JSEngine] Script content:\n%s\n", code);

    // Execute the script
    bool result = executeScript(ctx, code);
    delete[] code;

    return result;
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

    // Basic logging
    duk_push_c_function(duk_ctx, _js_log, 1);
    duk_put_global_string(duk_ctx, "log");

    // UI Creation
    duk_push_c_function(duk_ctx, _js_createLabel, 3);
    duk_put_global_string(duk_ctx, "createLabel");

    duk_push_c_function(duk_ctx, _js_updateLabel, 2);
    duk_put_global_string(duk_ctx, "updateLabel");

    duk_push_c_function(duk_ctx, _js_setLabelColor, 2);
    duk_put_global_string(duk_ctx, "setLabelColor");

    duk_push_c_function(duk_ctx, _js_setLabelSize, 2);
    duk_put_global_string(duk_ctx, "setLabelSize");

    duk_push_c_function(duk_ctx, _js_createButton, 3);
    duk_put_global_string(duk_ctx, "createButton");

    duk_push_c_function(duk_ctx, _js_setBackgroundColor, 1);
    duk_put_global_string(duk_ctx, "setBackgroundColor");

    duk_push_c_function(duk_ctx, _js_clearScreen, 0);
    duk_put_global_string(duk_ctx, "clearScreen");

    // Drawing
    duk_push_c_function(duk_ctx, _js_drawRectangle, 5);
    duk_put_global_string(duk_ctx, "drawRectangle");

    duk_push_c_function(duk_ctx, _js_drawCircle, 4);
    duk_put_global_string(duk_ctx, "drawCircle");

    // Advanced Text
    duk_push_c_function(duk_ctx, _js_createScrollingLabel, 4);
    duk_put_global_string(duk_ctx, "createScrollingLabel");

    duk_push_c_function(duk_ctx, _js_setTextAlign, 2);
    duk_put_global_string(duk_ctx, "setTextAlign");

    // Screen Info
    duk_push_c_function(duk_ctx, _js_getWidth, 0);
    duk_put_global_string(duk_ctx, "getWidth");

    duk_push_c_function(duk_ctx, _js_getHeight, 0);
    duk_put_global_string(duk_ctx, "getHeight");

    duk_push_c_function(duk_ctx, _js_getDisplayId, 0);
    duk_put_global_string(duk_ctx, "getDisplayId");

    // Text Styling
    duk_push_c_function(duk_ctx, _js_setTextColor, 2);
    duk_put_global_string(duk_ctx, "setTextColor");

    duk_push_c_function(duk_ctx, _js_setTextSize, 2);
    duk_put_global_string(duk_ctx, "setTextSize");

    // State Persistence
    duk_push_c_function(duk_ctx, _js_saveState, 2);
    duk_put_global_string(duk_ctx, "saveState");

    duk_push_c_function(duk_ctx, _js_loadState, 1);
    duk_put_global_string(duk_ctx, "loadState");

    // Time
    duk_push_c_function(duk_ctx, _js_millis, 0);
    duk_put_global_string(duk_ctx, "millis");

    duk_push_c_function(duk_ctx, _js_getTime, 0);
    duk_put_global_string(duk_ctx, "getTime");

    // HTTP
    duk_push_c_function(duk_ctx, _js_httpGet, 1);
    duk_put_global_string(duk_ctx, "httpGet");

    // Animations
    duk_push_c_function(duk_ctx, _js_fadeIn, 2);
    duk_put_global_string(duk_ctx, "fadeIn");

    duk_push_c_function(duk_ctx, _js_fadeOut, 2);
    duk_put_global_string(duk_ctx, "fadeOut");

    duk_push_c_function(duk_ctx, _js_moveLabel, 4);
    duk_put_global_string(duk_ctx, "moveLabel");

    duk_push_c_function(duk_ctx, _js_setOpacity, 2);
    duk_put_global_string(duk_ctx, "setOpacity");

    // Multi-Display
    duk_push_c_function(duk_ctx, _js_getDisplayCount, 0);
    duk_put_global_string(duk_ctx, "getDisplayCount");

    duk_push_c_function(duk_ctx, _js_sendToDisplay, 2);
    duk_put_global_string(duk_ctx, "sendToDisplay");

    // MQTT
    duk_push_c_function(duk_ctx, _js_mqttConnect, 3);
    duk_put_global_string(duk_ctx, "mqttConnect");

    duk_push_c_function(duk_ctx, _js_mqttPublish, 2);
    duk_put_global_string(duk_ctx, "mqttPublish");

    duk_push_c_function(duk_ctx, _js_mqttSubscribe, 1);
    duk_put_global_string(duk_ctx, "mqttSubscribe");

    duk_push_c_function(duk_ctx, _js_mqttDisconnect, 0);
    duk_put_global_string(duk_ctx, "mqttDisconnect");

    // WebSocket
    duk_push_c_function(duk_ctx, _js_wsConnect, 1);
    duk_put_global_string(duk_ctx, "wsConnect");

    duk_push_c_function(duk_ctx, _js_wsSend, 1);
    duk_put_global_string(duk_ctx, "wsSend");

    duk_push_c_function(duk_ctx, _js_wsOnMessage, 0);
    duk_put_global_string(duk_ctx, "wsOnMessage");

    duk_push_c_function(duk_ctx, _js_wsDisconnect, 0);
    duk_put_global_string(duk_ctx, "wsDisconnect");

    // Initialize display ID to 0 (can be overridden with setDisplayId)
    duk_push_global_stash(duk_ctx);
    duk_push_int(duk_ctx, 0);
    duk_put_prop_string(duk_ctx, -2, "__displayId");
    duk_pop(duk_ctx);

    Serial.println("[JSEngine] ✓ Registered Doki OS APIs (Advanced Features Enabled)");
#endif
}

void JSEngine::setDisplayId(void* ctx, uint8_t displayId) {
#ifdef ENABLE_JAVASCRIPT_SUPPORT
    if (!ctx) return;

    duk_context* duk_ctx = (duk_context*)ctx;

    // Store display ID in the global stash (hidden from JS code)
    duk_push_global_stash(duk_ctx);
    duk_push_int(duk_ctx, displayId);
    duk_put_prop_string(duk_ctx, -2, "__displayId");
    duk_pop(duk_ctx);

    Serial.printf("[JSEngine] Set display ID to %d for context\n", displayId);
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

    // Store object pointer in global stash array
    duk_push_global_stash(ctx);
    duk_get_prop_string(ctx, -1, "__lvgl_objects");

    // Create array if it doesn't exist
    if (!duk_is_array(ctx, -1)) {
        duk_pop(ctx);  // pop undefined
        duk_push_array(ctx);
        duk_dup(ctx, -1);  // duplicate array
        duk_put_prop_string(ctx, -3, "__lvgl_objects");  // store in stash
    }

    // Get next ID (array length)
    duk_size_t objId = duk_get_length(ctx, -1);

    // Store pointer at this ID
    duk_push_pointer(ctx, (void*)label);
    duk_put_prop_index(ctx, -2, objId);

    duk_pop_2(ctx);  // pop array and stash

    Serial.printf("[JS] Created label ID=%d: '%s' at (%d, %d)\n", (int)objId, text, x, y);

    // Return the ID
    duk_push_uint(ctx, objId);
    return 1;
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
    // Retrieve display ID from global stash
    duk_push_global_stash(ctx);
    duk_get_prop_string(ctx, -1, "__displayId");
    int displayId = duk_to_int(ctx, -1);
    duk_pop_2(ctx);  // Pop value and stash

    duk_push_int(ctx, displayId);
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

// Screen Info Functions
duk_ret_t JSEngine::_js_getWidth(duk_context* ctx) {
    // Get screen width (240 pixels for ST7789)
    lv_obj_t* screen = lv_scr_act();
    lv_coord_t width = lv_obj_get_width(screen);
    duk_push_int(ctx, width);
    return 1;
}

duk_ret_t JSEngine::_js_getHeight(duk_context* ctx) {
    // Get screen height (320 pixels for ST7789)
    lv_obj_t* screen = lv_scr_act();
    lv_coord_t height = lv_obj_get_height(screen);
    duk_push_int(ctx, height);
    return 1;
}

// Text Styling Functions
duk_ret_t JSEngine::_js_setTextColor(duk_context* ctx) {
    // For simplicity, we'll modify the last created label
    // In a more advanced version, we'd return object IDs from createLabel()
    uint32_t color = duk_to_uint32(ctx, 0);

    Serial.printf("[JS] setTextColor: 0x%06X (Note: applies to next created label)\n", color);

    // Store color in stash for next label creation
    duk_push_global_stash(ctx);
    duk_push_uint(ctx, color);
    duk_put_prop_string(ctx, -2, "__textColor");
    duk_pop(ctx);

    return 0;
}

duk_ret_t JSEngine::_js_setTextSize(duk_context* ctx) {
    int size = duk_to_int(ctx, 0);

    Serial.printf("[JS] setTextSize: %d (Note: applies to next created label)\n", size);

    // Store size in stash for next label creation
    duk_push_global_stash(ctx);
    duk_push_int(ctx, size);
    duk_put_prop_string(ctx, -2, "__textSize");
    duk_pop(ctx);

    return 0;
}

// Time Functions
duk_ret_t JSEngine::_js_millis(duk_context* ctx) {
    uint32_t ms = millis();
    duk_push_uint(ctx, ms);
    return 1;
}

duk_ret_t JSEngine::_js_getTime(duk_context* ctx) {
    // Lazy initialization - only initialize NTP when first called (after WiFi is connected)
    if (!ntpInitialized) {
        initNTPClient();  // Non-blocking initialization
    }

    // Check if NTP has synced yet
    if (!ntpSynced || !globalTimeClient) {
        // Still syncing in background, return null
        duk_push_null(ctx);
        return 1;
    }

    // Get epoch time (no update call needed, background task handles it)
    time_t epochTime = globalTimeClient->getEpochTime();

    if (epochTime < 1000000000) {
        // Not synced yet (shouldn't happen if ntpSynced is true, but safety check)
        duk_push_null(ctx);
        return 1;
    }

    // Convert to local time structure
    struct tm* timeInfo = localtime(&epochTime);

    // Create JavaScript object with time components
    duk_push_object(ctx);

    // Hours (24-hour format)
    duk_push_int(ctx, timeInfo->tm_hour);
    duk_put_prop_string(ctx, -2, "hour");

    // Minutes
    duk_push_int(ctx, timeInfo->tm_min);
    duk_put_prop_string(ctx, -2, "minute");

    // Seconds
    duk_push_int(ctx, timeInfo->tm_sec);
    duk_put_prop_string(ctx, -2, "second");

    // Day of month (1-31)
    duk_push_int(ctx, timeInfo->tm_mday);
    duk_put_prop_string(ctx, -2, "day");

    // Month (1-12, JavaScript style)
    duk_push_int(ctx, timeInfo->tm_mon + 1);
    duk_put_prop_string(ctx, -2, "month");

    // Year (full year)
    duk_push_int(ctx, timeInfo->tm_year + 1900);
    duk_put_prop_string(ctx, -2, "year");

    // Day of week (0=Sunday, 6=Saturday)
    duk_push_int(ctx, timeInfo->tm_wday);
    duk_put_prop_string(ctx, -2, "weekday");

    return 1;
}

// HTTP Function
duk_ret_t JSEngine::_js_httpGet(duk_context* ctx) {
    const char* url = duk_to_string(ctx, 0);

    Serial.printf("[JS] HTTP GET: %s\n", url);

    // Use HTTPClient to fetch data
    HTTPClient http;
    http.begin(url);
    http.setTimeout(5000);  // 5 second timeout

    int httpCode = http.GET();

    if (httpCode == HTTP_CODE_OK) {
        String payload = http.getString();
        Serial.printf("[JS] HTTP Response: %d bytes\n", payload.length());

        duk_push_string(ctx, payload.c_str());
        http.end();
        return 1;
    } else {
        Serial.printf("[JS] HTTP Error: %d\n", httpCode);
        http.end();
        duk_push_null(ctx);
        return 1;
    }
}

// Label Update Functions
duk_ret_t JSEngine::_js_updateLabel(duk_context* ctx) {
    duk_uint_t objId = duk_to_uint(ctx, 0);
    const char* newText = duk_to_string(ctx, 1);

    // Get object from stash
    duk_push_global_stash(ctx);
    duk_get_prop_string(ctx, -1, "__lvgl_objects");

    if (!duk_is_array(ctx, -1)) {
        Serial.println("[JS] ERROR: No objects array in stash");
        duk_pop_2(ctx);
        return 0;
    }

    duk_get_prop_index(ctx, -1, objId);
    lv_obj_t* obj = (lv_obj_t*)duk_to_pointer(ctx, -1);
    duk_pop_3(ctx);  // pop pointer, array, stash

    if (obj) {
        // Get display ID from stash for logging
        duk_push_global_stash(ctx);
        duk_get_prop_string(ctx, -1, "__displayId");
        int displayId = duk_to_int(ctx, -1);
        duk_pop_2(ctx);  // Pop value and stash

        lv_label_set_text(obj, newText);
        Serial.printf("[JS Display %d] Updated label ID=%d: '%s'\n", displayId, objId, newText);
    } else {
        Serial.printf("[JS] ERROR: Invalid object ID=%d\n", objId);
    }

    return 0;
}

duk_ret_t JSEngine::_js_setLabelColor(duk_context* ctx) {
    duk_uint_t objId = duk_to_uint(ctx, 0);
    uint32_t color = duk_to_uint32(ctx, 1);

    // Get object from stash
    duk_push_global_stash(ctx);
    duk_get_prop_string(ctx, -1, "__lvgl_objects");

    if (!duk_is_array(ctx, -1)) {
        duk_pop_2(ctx);
        return 0;
    }

    duk_get_prop_index(ctx, -1, objId);
    lv_obj_t* obj = (lv_obj_t*)duk_to_pointer(ctx, -1);
    duk_pop_3(ctx);

    if (obj) {
        lv_obj_set_style_text_color(obj, lv_color_hex(color), 0);
        Serial.printf("[JS] Set label ID=%d color: 0x%06X\n", objId, color);
    }

    return 0;
}

duk_ret_t JSEngine::_js_setLabelSize(duk_context* ctx) {
    duk_uint_t objId = duk_to_uint(ctx, 0);
    int size = duk_to_int(ctx, 1);

    // Get object from stash
    duk_push_global_stash(ctx);
    duk_get_prop_string(ctx, -1, "__lvgl_objects");

    if (!duk_is_array(ctx, -1)) {
        duk_pop_2(ctx);
        return 0;
    }

    duk_get_prop_index(ctx, -1, objId);
    lv_obj_t* obj = (lv_obj_t*)duk_to_pointer(ctx, -1);
    duk_pop_3(ctx);

    if (obj) {
        const lv_font_t* font = &lv_font_montserrat_14;  // default

        if (size == 12) font = &lv_font_montserrat_12;
        else if (size == 14) font = &lv_font_montserrat_14;
        else if (size == 16) font = &lv_font_montserrat_16;
        else if (size == 20) font = &lv_font_montserrat_20;
        else if (size == 24) font = &lv_font_montserrat_24;

        lv_obj_set_style_text_font(obj, font, 0);
        Serial.printf("[JS] Set label ID=%d font size: %d\n", objId, size);
    }

    return 0;
}

// Screen Management
duk_ret_t JSEngine::_js_clearScreen(duk_context* ctx) {
    lv_obj_t* screen = lv_scr_act();

    // Delete all children
    lv_obj_clean(screen);

    // Clear object array in stash
    duk_push_global_stash(ctx);
    duk_push_array(ctx);
    duk_put_prop_string(ctx, -2, "__lvgl_objects");
    duk_pop(ctx);

    Serial.println("[JS] Cleared screen");
    return 0;
}

// Drawing Functions
duk_ret_t JSEngine::_js_drawRectangle(duk_context* ctx) {
    int x = duk_to_int(ctx, 0);
    int y = duk_to_int(ctx, 1);
    int w = duk_to_int(ctx, 2);
    int h = duk_to_int(ctx, 3);
    uint32_t color = duk_to_uint32(ctx, 4);

    lv_obj_t* rect = lv_obj_create(lv_scr_act());
    lv_obj_set_size(rect, w, h);
    lv_obj_set_pos(rect, x, y);
    lv_obj_set_style_bg_color(rect, lv_color_hex(color), 0);
    lv_obj_set_style_border_width(rect, 0, 0);
    lv_obj_set_style_radius(rect, 0, 0);

    Serial.printf("[JS] Drew rectangle: (%d,%d) %dx%d color=0x%06X\n", x, y, w, h, color);
    return 0;
}

duk_ret_t JSEngine::_js_drawCircle(duk_context* ctx) {
    int x = duk_to_int(ctx, 0);
    int y = duk_to_int(ctx, 1);
    int radius = duk_to_int(ctx, 2);
    uint32_t color = duk_to_uint32(ctx, 3);

    lv_obj_t* circle = lv_obj_create(lv_scr_act());
    int diameter = radius * 2;
    lv_obj_set_size(circle, diameter, diameter);
    lv_obj_set_pos(circle, x - radius, y - radius);  // Center it
    lv_obj_set_style_bg_color(circle, lv_color_hex(color), 0);
    lv_obj_set_style_border_width(circle, 0, 0);
    lv_obj_set_style_radius(circle, LV_RADIUS_CIRCLE, 0);

    Serial.printf("[JS] Drew circle: (%d,%d) radius=%d color=0x%06X\n", x, y, radius, color);
    return 0;
}

// Advanced Text Functions
duk_ret_t JSEngine::_js_createScrollingLabel(duk_context* ctx) {
    const char* text = duk_to_string(ctx, 0);
    int x = duk_to_int(ctx, 1);
    int y = duk_to_int(ctx, 2);
    int width = duk_to_int(ctx, 3);

    lv_obj_t* label = lv_label_create(lv_scr_act());
    lv_label_set_text(label, text);
    lv_obj_set_pos(label, x, y);
    lv_obj_set_width(label, width);
    lv_label_set_long_mode(label, LV_LABEL_LONG_SCROLL_CIRCULAR);

    // Store in stash and return ID
    duk_push_global_stash(ctx);
    duk_get_prop_string(ctx, -1, "__lvgl_objects");

    if (!duk_is_array(ctx, -1)) {
        duk_pop(ctx);
        duk_push_array(ctx);
        duk_dup(ctx, -1);
        duk_put_prop_string(ctx, -3, "__lvgl_objects");
    }

    duk_size_t objId = duk_get_length(ctx, -1);
    duk_push_pointer(ctx, (void*)label);
    duk_put_prop_index(ctx, -2, objId);
    duk_pop_2(ctx);

    Serial.printf("[JS] Created scrolling label ID=%d: '%s' width=%d\n", (int)objId, text, width);

    duk_push_uint(ctx, objId);
    return 1;
}

duk_ret_t JSEngine::_js_setTextAlign(duk_context* ctx) {
    duk_uint_t objId = duk_to_uint(ctx, 0);
    int align = duk_to_int(ctx, 1);  // 0=left, 1=center, 2=right

    // Get object from stash
    duk_push_global_stash(ctx);
    duk_get_prop_string(ctx, -1, "__lvgl_objects");

    if (!duk_is_array(ctx, -1)) {
        duk_pop_2(ctx);
        return 0;
    }

    duk_get_prop_index(ctx, -1, objId);
    lv_obj_t* obj = (lv_obj_t*)duk_to_pointer(ctx, -1);
    duk_pop_3(ctx);

    if (obj) {
        lv_text_align_t lv_align = LV_TEXT_ALIGN_LEFT;
        if (align == 1) lv_align = LV_TEXT_ALIGN_CENTER;
        else if (align == 2) lv_align = LV_TEXT_ALIGN_RIGHT;

        lv_obj_set_style_text_align(obj, lv_align, 0);
        Serial.printf("[JS] Set label ID=%d text align: %d\n", objId, align);
    }

    return 0;
}

// ========================================
// Advanced Features: Animations
// ========================================

duk_ret_t JSEngine::_js_fadeIn(duk_context* ctx) {
    duk_uint_t objId = duk_to_uint(ctx, 0);
    int duration = duk_to_int(ctx, 1);

    // Get object from stash
    duk_push_global_stash(ctx);
    duk_get_prop_string(ctx, -1, "__lvgl_objects");

    if (!duk_is_array(ctx, -1)) {
        duk_pop_2(ctx);
        return 0;
    }

    duk_get_prop_index(ctx, -1, objId);
    lv_obj_t* obj = (lv_obj_t*)duk_to_pointer(ctx, -1);
    duk_pop_3(ctx);

    if (obj) {
        lv_anim_t anim;
        lv_anim_init(&anim);
        lv_anim_set_var(&anim, obj);
        lv_anim_set_values(&anim, 0, 255);
        lv_anim_set_time(&anim, duration);
        lv_anim_set_exec_cb(&anim, (lv_anim_exec_xcb_t)lv_obj_set_style_opa);
        lv_anim_start(&anim);

        Serial.printf("[JS] Fade in ID=%d duration=%dms\n", objId, duration);
    }

    return 0;
}

duk_ret_t JSEngine::_js_fadeOut(duk_context* ctx) {
    duk_uint_t objId = duk_to_uint(ctx, 0);
    int duration = duk_to_int(ctx, 1);

    // Get object from stash
    duk_push_global_stash(ctx);
    duk_get_prop_string(ctx, -1, "__lvgl_objects");

    if (!duk_is_array(ctx, -1)) {
        duk_pop_2(ctx);
        return 0;
    }

    duk_get_prop_index(ctx, -1, objId);
    lv_obj_t* obj = (lv_obj_t*)duk_to_pointer(ctx, -1);
    duk_pop_3(ctx);

    if (obj) {
        lv_anim_t anim;
        lv_anim_init(&anim);
        lv_anim_set_var(&anim, obj);
        lv_anim_set_values(&anim, 255, 0);
        lv_anim_set_time(&anim, duration);
        lv_anim_set_exec_cb(&anim, (lv_anim_exec_xcb_t)lv_obj_set_style_opa);
        lv_anim_start(&anim);

        Serial.printf("[JS] Fade out ID=%d duration=%dms\n", objId, duration);
    }

    return 0;
}

static void _anim_x_cb(void* var, int32_t v) {
    lv_obj_set_x((lv_obj_t*)var, v);
}

static void _anim_y_cb(void* var, int32_t v) {
    lv_obj_set_y((lv_obj_t*)var, v);
}

duk_ret_t JSEngine::_js_moveLabel(duk_context* ctx) {
    duk_uint_t objId = duk_to_uint(ctx, 0);
    int targetX = duk_to_int(ctx, 1);
    int targetY = duk_to_int(ctx, 2);
    int duration = duk_to_int(ctx, 3);

    // Get object from stash
    duk_push_global_stash(ctx);
    duk_get_prop_string(ctx, -1, "__lvgl_objects");

    if (!duk_is_array(ctx, -1)) {
        duk_pop_2(ctx);
        return 0;
    }

    duk_get_prop_index(ctx, -1, objId);
    lv_obj_t* obj = (lv_obj_t*)duk_to_pointer(ctx, -1);
    duk_pop_3(ctx);

    if (obj) {
        int currentX = lv_obj_get_x(obj);
        int currentY = lv_obj_get_y(obj);

        // X animation
        lv_anim_t anim_x;
        lv_anim_init(&anim_x);
        lv_anim_set_var(&anim_x, obj);
        lv_anim_set_values(&anim_x, currentX, targetX);
        lv_anim_set_time(&anim_x, duration);
        lv_anim_set_exec_cb(&anim_x, (lv_anim_exec_xcb_t)_anim_x_cb);
        lv_anim_start(&anim_x);

        // Y animation
        lv_anim_t anim_y;
        lv_anim_init(&anim_y);
        lv_anim_set_var(&anim_y, obj);
        lv_anim_set_values(&anim_y, currentY, targetY);
        lv_anim_set_time(&anim_y, duration);
        lv_anim_set_exec_cb(&anim_y, (lv_anim_exec_xcb_t)_anim_y_cb);
        lv_anim_start(&anim_y);

        Serial.printf("[JS] Move ID=%d to (%d,%d) duration=%dms\n", objId, targetX, targetY, duration);
    }

    return 0;
}

duk_ret_t JSEngine::_js_setOpacity(duk_context* ctx) {
    duk_uint_t objId = duk_to_uint(ctx, 0);
    int opacity = duk_to_int(ctx, 1);  // 0-255

    // Get object from stash
    duk_push_global_stash(ctx);
    duk_get_prop_string(ctx, -1, "__lvgl_objects");

    if (!duk_is_array(ctx, -1)) {
        duk_pop_2(ctx);
        return 0;
    }

    duk_get_prop_index(ctx, -1, objId);
    lv_obj_t* obj = (lv_obj_t*)duk_to_pointer(ctx, -1);
    duk_pop_3(ctx);

    if (obj) {
        lv_obj_set_style_opa(obj, opacity, 0);
        Serial.printf("[JS] Set opacity ID=%d: %d\n", objId, opacity);
    }

    return 0;
}

// ========================================
// Advanced Features: Multi-Display Coordination
// ========================================

duk_ret_t JSEngine::_js_getDisplayCount(duk_context* ctx) {
    uint8_t count = AppManager::getNumDisplays();
    duk_push_uint(ctx, count);
    return 1;
}

duk_ret_t JSEngine::_js_sendToDisplay(duk_context* ctx) {
    uint8_t targetDisplayId = duk_to_uint(ctx, 0);
    const char* message = duk_to_string(ctx, 1);

    // Store message in global stash for target display
    // This is a simplified approach - messages stored per display
    duk_push_global_stash(ctx);

    // Create messages object if it doesn't exist
    duk_get_prop_string(ctx, -1, "__display_messages");
    if (!duk_is_object(ctx, -1)) {
        duk_pop(ctx);
        duk_push_object(ctx);
        duk_dup(ctx, -1);
        duk_put_prop_string(ctx, -3, "__display_messages");
    }

    // Store message for target display
    char key[16];
    snprintf(key, sizeof(key), "disp_%d", targetDisplayId);
    duk_push_string(ctx, message);
    duk_put_prop_string(ctx, -2, key);

    duk_pop_2(ctx);  // pop messages object and stash

    Serial.printf("[JS] Sent message to display %d: %s\n", targetDisplayId, message);
    return 0;
}

// ========================================
// Advanced Features: MQTT Support
// ========================================

// Global MQTT client (one per JS context)
static WiFiClient mqttWifiClient;
static PubSubClient* mqttClient = nullptr;
static duk_context* mqttDukContext = nullptr;

static void mqttCallback(char* topic, byte* payload, unsigned int length) {
    if (!mqttDukContext) return;

    Serial.printf("[MQTT] Message on topic '%s'\n", topic);

    // Store message in stash for JavaScript to retrieve
    duk_push_global_stash(mqttDukContext);
    duk_get_prop_string(mqttDukContext, -1, "__mqtt_messages");

    if (!duk_is_array(mqttDukContext, -1)) {
        duk_pop(mqttDukContext);
        duk_push_array(mqttDukContext);
        duk_dup(mqttDukContext, -1);
        duk_put_prop_string(mqttDukContext, -3, "__mqtt_messages");
    }

    // Add message to array
    duk_idx_t arrIdx = duk_get_length(mqttDukContext, -1);
    duk_push_object(mqttDukContext);

    duk_push_string(mqttDukContext, topic);
    duk_put_prop_string(mqttDukContext, -2, "topic");

    String payloadStr = "";
    for (unsigned int i = 0; i < length; i++) {
        payloadStr += (char)payload[i];
    }
    duk_push_string(mqttDukContext, payloadStr.c_str());
    duk_put_prop_string(mqttDukContext, -2, "message");

    duk_put_prop_index(mqttDukContext, -2, arrIdx);

    duk_pop_2(mqttDukContext);  // pop array and stash
}

duk_ret_t JSEngine::_js_mqttConnect(duk_context* ctx) {
    const char* broker = duk_to_string(ctx, 0);
    int port = duk_to_int(ctx, 1);
    const char* clientId = duk_to_string(ctx, 2);

    Serial.printf("[MQTT] Attempting connection to %s:%d as '%s'...\n", broker, port, clientId);

    if (!mqttClient) {
        mqttClient = new PubSubClient(mqttWifiClient);
    }

    mqttDukContext = ctx;
    mqttClient->setServer(broker, port);
    mqttClient->setCallback(mqttCallback);

    // Set socket timeout to prevent blocking (default is too long)
    mqttClient->setSocketTimeout(5);  // 5 seconds timeout

    // Attempt connection (this may still block briefly, but with timeout)
    bool connected = mqttClient->connect(clientId);

    if (connected) {
        Serial.println("[MQTT] ✓ Connected successfully");
    } else {
        Serial.printf("[MQTT] ✗ Connection failed (state: %d)\n", mqttClient->state());
    }

    duk_push_boolean(ctx, connected);
    return 1;
}

duk_ret_t JSEngine::_js_mqttPublish(duk_context* ctx) {
    const char* topic = duk_to_string(ctx, 0);
    const char* message = duk_to_string(ctx, 1);

    if (!mqttClient || !mqttClient->connected()) {
        Serial.println("[MQTT] Not connected");
        duk_push_boolean(ctx, false);
        return 1;
    }

    mqttClient->loop();  // Process incoming messages
    bool success = mqttClient->publish(topic, message);

    Serial.printf("[MQTT] Publish to '%s': %s\n", topic, message);

    duk_push_boolean(ctx, success);
    return 1;
}

duk_ret_t JSEngine::_js_mqttSubscribe(duk_context* ctx) {
    const char* topic = duk_to_string(ctx, 0);

    if (!mqttClient || !mqttClient->connected()) {
        Serial.println("[MQTT] Not connected");
        duk_push_boolean(ctx, false);
        return 1;
    }

    bool success = mqttClient->subscribe(topic);

    Serial.printf("[MQTT] Subscribe to '%s': %s\n", topic, success ? "OK" : "FAILED");

    duk_push_boolean(ctx, success);
    return 1;
}

duk_ret_t JSEngine::_js_mqttDisconnect(duk_context* ctx) {
    if (mqttClient) {
        mqttClient->disconnect();
        Serial.println("[MQTT] Disconnected");
    }
    return 0;
}

// ========================================
// Advanced Features: WebSocket Support
// ========================================

#ifdef ENABLE_WEBSOCKET_SUPPORT
using namespace websockets;

static WebsocketsClient* wsClient = nullptr;
static duk_context* wsDukContext = nullptr;

static void wsMessageCallback(WebsocketsMessage message) {
    if (!wsDukContext) return;

    Serial.printf("[WebSocket] Message: %s\n", message.data().c_str());

    // Store message in stash
    duk_push_global_stash(wsDukContext);
    duk_get_prop_string(wsDukContext, -1, "__ws_messages");

    if (!duk_is_array(wsDukContext, -1)) {
        duk_pop(wsDukContext);
        duk_push_array(wsDukContext);
        duk_dup(wsDukContext, -1);
        duk_put_prop_string(wsDukContext, -3, "__ws_messages");
    }

    duk_idx_t arrIdx = duk_get_length(wsDukContext, -1);
    duk_push_string(wsDukContext, message.data().c_str());
    duk_put_prop_index(wsDukContext, -2, arrIdx);

    duk_pop_2(wsDukContext);  // pop array and stash
}
#endif

duk_ret_t JSEngine::_js_wsConnect(duk_context* ctx) {
#ifdef ENABLE_WEBSOCKET_SUPPORT
    const char* url = duk_to_string(ctx, 0);

    if (!wsClient) {
        wsClient = new WebsocketsClient();
    }

    wsDukContext = ctx;
    wsClient->onMessage(wsMessageCallback);

    bool connected = wsClient->connect(url);

    Serial.printf("[WebSocket] Connect to %s: %s\n", url, connected ? "OK" : "FAILED");

    duk_push_boolean(ctx, connected);
    return 1;
#else
    Serial.println("[WebSocket] Not supported - enable ENABLE_WEBSOCKET_SUPPORT in js_engine.cpp");
    duk_push_boolean(ctx, false);
    return 1;
#endif
}

duk_ret_t JSEngine::_js_wsSend(duk_context* ctx) {
#ifdef ENABLE_WEBSOCKET_SUPPORT
    const char* message = duk_to_string(ctx, 0);

    if (!wsClient || !wsClient->available()) {
        Serial.println("[WebSocket] Not connected");
        duk_push_boolean(ctx, false);
        return 1;
    }

    wsClient->poll();  // Process events
    wsClient->send(message);

    Serial.printf("[WebSocket] Sent: %s\n", message);

    duk_push_boolean(ctx, true);
    return 1;
#else
    duk_push_boolean(ctx, false);
    return 1;
#endif
}

duk_ret_t JSEngine::_js_wsOnMessage(duk_context* ctx) {
#ifdef ENABLE_WEBSOCKET_SUPPORT
    // This is handled by the callback system
    // JavaScript polls messages from stash array
    if (wsClient) {
        wsClient->poll();  // Process pending messages
    }
    Serial.println("[WebSocket] Polling for messages");
#endif
    return 0;
}

duk_ret_t JSEngine::_js_wsDisconnect(duk_context* ctx) {
#ifdef ENABLE_WEBSOCKET_SUPPORT
    if (wsClient) {
        wsClient->close();
        delete wsClient;
        wsClient = nullptr;
        Serial.println("[WebSocket] Disconnected");
    }
#endif
    return 0;
}

#endif // ENABLE_JAVASCRIPT_SUPPORT

} // namespace Doki
