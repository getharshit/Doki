/**
 * @file js_app.cpp
 * @brief Implementation of JavaScript App Wrapper
 */

#include "doki/js_app.h"
#include <lvgl.h>

namespace Doki {

JSApp::JSApp(const char* id, const char* name, const char* scriptPath)
    : DokiApp(id, name),
      _scriptPath(scriptPath),
      _jsContext(nullptr),
      _scriptLoaded(false),
      _lastUpdate(0)
{
    Serial.printf("[JSApp:%s] Created with script: %s\n", id, scriptPath);
}

JSApp::~JSApp() {
    if (_jsContext) {
        JSEngine::destroyContext(_jsContext);
        _jsContext = nullptr;
    }
}

void JSApp::onCreate() {
    log("Creating JavaScript app...");

    // Check if JavaScript support is enabled
    if (!JSEngine::isEnabled()) {
        _showError("JavaScript not enabled\n\nSee docs for setup");
        log("ERROR: JavaScript support not compiled in");
        return;
    }

    // Create JS context
    _jsContext = JSEngine::createContext();
    if (!_jsContext) {
        _showError("Failed to create\nJS context");
        log("ERROR: Failed to create JS context");
        return;
    }

    // Load and execute the script
    if (!JSEngine::loadScript(_jsContext, _scriptPath.c_str())) {
        String error = "Script error:\n" + String(JSEngine::getLastError());
        _showError(error.c_str());
        log(String("ERROR: " + String(JSEngine::getLastError())).c_str());
        return;
    }

    _scriptLoaded = true;

    // Call JavaScript onCreate()
    if (!JSEngine::callFunction(_jsContext, "onCreate")) {
        String error = "onCreate() error:\n" + String(JSEngine::getLastError());
        _showError(error.c_str());
        return;
    }

    log("✓ JavaScript app created successfully");
}

void JSApp::onStart() {
    log("JavaScript app started");

    if (_scriptLoaded && _jsContext) {
        JSEngine::callFunction(_jsContext, "onStart");
    }
}

void JSApp::onUpdate() {
    if (!_scriptLoaded || !_jsContext) {
        return;
    }

    // Throttle updates to avoid overloading
    uint32_t now = millis();
    if (now - _lastUpdate < UPDATE_INTERVAL) {
        return;
    }
    _lastUpdate = now;

    // Call JavaScript onUpdate()
    JSEngine::callFunction(_jsContext, "onUpdate");
}

void JSApp::onPause() {
    log("JavaScript app paused");

    if (_scriptLoaded && _jsContext) {
        JSEngine::callFunction(_jsContext, "onPause");
    }
}

void JSApp::onDestroy() {
    log("JavaScript app destroyed");

    if (_scriptLoaded && _jsContext) {
        JSEngine::callFunction(_jsContext, "onDestroy");
    }

    // Cleanup JS context
    if (_jsContext) {
        JSEngine::destroyContext(_jsContext);
        _jsContext = nullptr;
    }

    _scriptLoaded = false;
}

void JSApp::onSaveState(JsonDocument& state) {
    if (!_scriptLoaded || !_jsContext) {
        return;
    }

    // Call JavaScript onSaveState() if it exists
    // The JS function should return an object with state data
    JSEngine::callFunction(_jsContext, "onSaveState");

    // For now, just save the script path
    state["scriptPath"] = _scriptPath;
}

void JSApp::onRestoreState(const JsonDocument& state) {
    if (!_scriptLoaded || !_jsContext) {
        return;
    }

    // Call JavaScript onRestoreState() with state data
    JSEngine::callFunctionWithArgs(_jsContext, "onRestoreState", state);
}

void JSApp::_showError(const char* message) {
    // Black background
    lv_obj_set_style_bg_color(getScreen(), lv_color_hex(0x000000), 0);

    // Create error label
    lv_obj_t* label = lv_label_create(getScreen());
    lv_label_set_text(label, message);
    lv_obj_set_style_text_color(label, lv_color_hex(0xFF0000), 0);
    lv_obj_set_style_text_font(label, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_align(label, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_align(label, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_width(label, 200);

    // Create "JavaScript App" title
    lv_obj_t* title = lv_label_create(getScreen());
    lv_label_set_text(title, "JavaScript App");
    lv_obj_set_style_text_color(title, lv_color_hex(0x888888), 0);
    lv_obj_set_style_text_font(title, &lv_font_montserrat_12, 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 20);
}

} // namespace Doki
