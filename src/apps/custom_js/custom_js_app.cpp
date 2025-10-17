/**
 * @file custom_js_app.cpp
 * @brief Implementation of Custom JavaScript App
 */

#include "custom_js_app.h"
#include "doki/filesystem_manager.h"
#include "doki/app_manager.h"
#include <lvgl.h>

CustomJSApp::CustomJSApp()
    : JSApp("custom", "Custom JS", "/js/custom_disp0.js")  // Default path, will be updated
    , _lastWatchdogReset(0)
    , _creationStartTime(0)
{
    // Update script path based on which display this app is running on
    // This will be determined when setDisplay() is called by AppManager
}

CustomJSApp::~CustomJSApp() {
    // Destructor
}

void CustomJSApp::onCreate() {
    _creationStartTime = millis();
    _resetWatchdog();

    // Determine which display we're on and update the script path
    uint8_t displayId = getDisplayId();
    char scriptPath[32];
    snprintf(scriptPath, sizeof(scriptPath), "/js/custom_disp%d.js", displayId);
    _scriptPath = scriptPath;

    Serial.printf("[CustomJSApp] Loading custom JS for display %d: %s\n", displayId, scriptPath);

    // Check if JS engine is available
    if (!Doki::JSEngine::isEnabled()) {
        Serial.println("[CustomJSApp] JavaScript support is disabled");

        // Show informative message
        lv_obj_set_style_bg_color(lv_scr_act(), lv_color_hex(0x1a1a2e), 0);

        lv_obj_t* title = lv_label_create(lv_scr_act());
        lv_label_set_text(title, "JavaScript");
        lv_obj_set_style_text_font(title, &lv_font_montserrat_24, 0);
        lv_obj_set_style_text_color(title, lv_color_hex(0x667EEA), 0);
        lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 30);

        lv_obj_t* status = lv_label_create(lv_scr_act());
        lv_label_set_text(status, "Coming Soon!");
        lv_obj_set_style_text_font(status, &lv_font_montserrat_20, 0);
        lv_obj_set_style_text_color(status, lv_color_hex(0x10B981), 0);
        lv_obj_align(status, LV_ALIGN_TOP_MID, 0, 65);

        lv_obj_t* info = lv_label_create(lv_scr_act());
        char infoText[256];
        if (hasCustomCode()) {
            snprintf(infoText, sizeof(infoText),
                "Your code is saved:\n\n"
                "%s\n\n"
                "Enable JS in\n"
                "js_engine.h to run it", scriptPath);
        } else {
            snprintf(infoText, sizeof(infoText),
                "Upload code via\n"
                "web dashboard:\n\n"
                "http://192.168.x.x\n\n"
                "(JS execution will be\n"
                "enabled in future)");
        }
        lv_label_set_text(info, infoText);
        lv_obj_set_style_text_align(info, LV_TEXT_ALIGN_CENTER, 0);
        lv_obj_set_style_text_color(info, lv_color_white(), 0);
        lv_obj_center(info);
        return;
    }

    // Check if custom JS file exists
    if (!hasCustomCode()) {
        Serial.printf("[CustomJSApp] No custom code found at %s\n", scriptPath);

        // Show helpful message
        lv_obj_t* label = lv_label_create(lv_scr_act());
        lv_label_set_text(label,
            "No Custom Code\n\n"
            "Upload JavaScript via\n"
            "web dashboard to get\n"
            "started!\n\n"
            "http://192.168.x.x");
        lv_obj_set_style_text_align(label, LV_TEXT_ALIGN_CENTER, 0);
        lv_obj_center(label);
        return;
    }

    // Call parent onCreate to load and execute the script
    JSApp::onCreate();

    // Check for watchdog timeout during onCreate
    if (_watchdogTimeout()) {
        Serial.println("[CustomJSApp] ✗ Watchdog timeout during onCreate!");
        _showError("Script timeout\n(infinite loop?)");
    }

    _resetWatchdog();
}

void CustomJSApp::onUpdate() {
    // Reset watchdog before update
    _resetWatchdog();

    // Call parent onUpdate
    JSApp::onUpdate();

    // Check for timeout after update
    if (_watchdogTimeout()) {
        Serial.println("[CustomJSApp] ✗ Watchdog timeout during onUpdate!");
        // Don't show error repeatedly, just log it
    }
}

bool CustomJSApp::hasCustomCode() {
    uint8_t displayId = getDisplayId();
    char scriptPath[32];
    snprintf(scriptPath, sizeof(scriptPath), "/js/custom_disp%d.js", displayId);

    // Check if file exists and has content
    if (!Doki::FilesystemManager::exists(scriptPath)) {
        return false;
    }

    size_t size = Doki::FilesystemManager::getFileSize(scriptPath);
    return size > 0;
}

void CustomJSApp::_resetWatchdog() {
    _lastWatchdogReset = millis();
}

bool CustomJSApp::_watchdogTimeout() {
    uint32_t elapsed = millis() - _lastWatchdogReset;
    return elapsed > WATCHDOG_TIMEOUT;
}
