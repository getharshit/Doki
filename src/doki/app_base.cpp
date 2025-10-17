/**
 * @file app_base.cpp
 * @brief Implementation of DokiApp base class
 */

#include "doki/app_base.h"
#include "doki/app_manager.h"

namespace Doki {

// ========================================
// Constructor & Destructor
// ========================================

DokiApp::DokiApp(const char* id, const char* name)
    : _id(id)
    , _name(name)
    , _display(nullptr)
    , _state(AppState::IDLE)
    , _startTime(0)
{
    // Constructor - app is in IDLE state
    Serial.printf("[DokiApp] Created app: %s (%s)\n", _name, _id);
}

DokiApp::~DokiApp() {
    // Destructor - ensure cleanup
    Serial.printf("[DokiApp] Destroyed app: %s\n", _name);
}

// ========================================
// Public Methods (Lifecycle Management)
// ========================================

void DokiApp::_setState(AppState state) {
    AppState oldState = _state;
    _state = state;
    
    // Log state transitions
    const char* stateNames[] = {
        "IDLE", "CREATED", "STARTED", "PAUSED", "DESTROYED"
    };
    
    Serial.printf("[DokiApp:%s] State: %s -> %s\n", 
                  _id, 
                  stateNames[(int)oldState], 
                  stateNames[(int)state]);
}

void DokiApp::_markStarted() {
    _startTime = millis();
}

// ========================================
// Public Methods (App Info)
// ========================================

void DokiApp::setDisplay(lv_disp_t* disp) {
    if (!disp) {
        Serial.printf("[DokiApp:%s] Error: Cannot set null display\n", _id);
        return;
    }
    _display = disp;
    // Set this display as the default for LVGL operations
    lv_disp_set_default(disp);
    Serial.printf("[DokiApp:%s] Display assigned\n", _id);
}

uint32_t DokiApp::getUptime() const {
    if (_state != AppState::STARTED || _startTime == 0) {
        return 0;
    }
    return millis() - _startTime;
}

uint8_t DokiApp::getDisplayId() const {
    // Call AppManager to find which display this app is on
    return AppManager::getDisplayIdForApp(const_cast<DokiApp*>(this));
}

// ========================================
// Protected Helper Methods
// ========================================

void DokiApp::clearScreen() {
    if (!_display) {
        Serial.printf("[DokiApp:%s] Error: No display assigned, cannot clear screen\n", _id);
        return;
    }

    // Get the active screen for this specific display
    lv_obj_t* screen = lv_disp_get_scr_act(_display);
    if (!screen) {
        Serial.printf("[DokiApp:%s] Error: No active screen found\n", _id);
        return;
    }

    // Clean all children from the screen
    lv_obj_clean(screen);
    Serial.printf("[DokiApp:%s] Screen cleared\n", _id);
}

void DokiApp::log(const char* message) {
    Serial.printf("[DokiApp:%s] %s\n", _id, message);
}

} // namespace Doki