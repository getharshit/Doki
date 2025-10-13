/**
 * @file app_base.cpp
 * @brief Implementation of DokiApp base class
 */

#include "doki/app_base.h"

namespace Doki {

// ========================================
// Constructor & Destructor
// ========================================

DokiApp::DokiApp(const char* id, const char* name)
    : _id(id)
    , _name(name)
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

uint32_t DokiApp::getUptime() const {
    if (_state != AppState::STARTED || _startTime == 0) {
        return 0;
    }
    return millis() - _startTime;
}

// ========================================
// Protected Helper Methods
// ========================================

void DokiApp::clearScreen() {
    // Clean all children from the active screen
    lv_obj_clean(lv_scr_act());
    Serial.printf("[DokiApp:%s] Screen cleared\n", _id);
}

void DokiApp::log(const char* message) {
    Serial.printf("[DokiApp:%s] %s\n", _id, message);
}

} // namespace Doki