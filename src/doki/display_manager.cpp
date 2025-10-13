/**
 * @file display_manager.cpp
 * @brief Implementation of Display Manager for Doki OS
 */

#include "doki/display_manager.h"
#include <SPI.h>

namespace Doki {

// ========================================
// Static Member Initialization
// ========================================

DisplayInfo DisplayManager::_displays[MAX_DISPLAYS];
uint8_t DisplayManager::_displayCount = 0;
bool DisplayManager::_initialized = false;

// ========================================
// Public Methods
// ========================================

bool DisplayManager::init(uint8_t displayCount) {
    if (_initialized) {
        Serial.println("[DisplayManager] Already initialized");
        return true;
    }
    
    if (displayCount > MAX_DISPLAYS) {
        Serial.printf("[DisplayManager] Error: Max %d displays supported\n", MAX_DISPLAYS);
        return false;
    }
    
    Serial.printf("[DisplayManager] Initializing %d display(s)...\n", displayCount);
    
    _displayCount = displayCount;
    
    // Initialize each display
    for (uint8_t i = 0; i < displayCount; i++) {
        DisplayConfig config = _getDefaultConfig(i);
        
        Serial.printf("[DisplayManager] Initializing Display %d...\n", i);
        Serial.printf("  CS: %d, DC: %d, RST: %d\n", 
                      config.cs_pin, config.dc_pin, config.rst_pin);
        
        if (!_initDisplay(i, config)) {
            Serial.printf("[DisplayManager] Failed to initialize Display %d\n", i);
            return false;
        }
        
        _displays[i].id = i;
        _displays[i].config = config;
        _displays[i].initialized = true;
        
        Serial.printf("[DisplayManager] ✓ Display %d ready\n", i);
    }
    
    _initialized = true;
    Serial.printf("[DisplayManager] ✓ All %d display(s) initialized\n\n", displayCount);
    
    return true;
}

DisplayConfig DisplayManager::getDisplayConfig(uint8_t displayId) {
    if (displayId < _displayCount) {
        return _displays[displayId].config;
    }
    return DisplayConfig();
}

bool DisplayManager::isDisplayReady(uint8_t displayId) {
    return displayId < _displayCount && _displays[displayId].initialized;
}

uint8_t DisplayManager::getDisplayCount() {
    return _displayCount;
}

bool DisplayManager::assignApp(uint8_t displayId, DokiApp* app) {
    if (displayId >= _displayCount) {
        Serial.printf("[DisplayManager] Error: Display %d not available\n", displayId);
        return false;
    }
    
    if (!_displays[displayId].initialized) {
        Serial.printf("[DisplayManager] Error: Display %d not initialized\n", displayId);
        return false;
    }
    
    // Set LVGL to use this display's screen
    lv_disp_set_default(_displays[displayId].lvgl_display);
    
    // Update current app
    _displays[displayId].currentApp = app;
    
    if (app) {
        Serial.printf("[DisplayManager] App '%s' assigned to Display %d\n", 
                      app->getName(), displayId);
    } else {
        Serial.printf("[DisplayManager] Display %d cleared\n", displayId);
    }
    
    return true;
}

DokiApp* DisplayManager::getCurrentApp(uint8_t displayId) {
    if (displayId < _displayCount) {
        return _displays[displayId].currentApp;
    }
    return nullptr;
}

void DisplayManager::updateAll() {
    // Update LVGL for all displays
    lv_timer_handler();
    
    // Update apps on each display
    for (uint8_t i = 0; i < _displayCount; i++) {
        if (_displays[i].currentApp && _displays[i].currentApp->isRunning()) {
            // Switch LVGL to this display
            lv_disp_set_default(_displays[i].lvgl_display);
            
            // Update the app
            _displays[i].currentApp->onUpdate();
        }
    }
}

lv_disp_t* DisplayManager::getLvglDisplay(uint8_t displayId) {
    if (displayId < _displayCount) {
        return _displays[displayId].lvgl_display;
    }
    return nullptr;
}

void DisplayManager::printStatus() {
    Serial.println("┌─────────────────────────────────────┐");
    Serial.println("│ Display Manager Status              │");
    Serial.println("├─────────────────────────────────────┤");
    Serial.printf("│ Total displays: %d                    \n", _displayCount);
    Serial.println("│                                     │");
    
    for (uint8_t i = 0; i < _displayCount; i++) {
        Serial.printf("│ Display %d:                          \n", i);
        Serial.printf("│   CS: %d, DC: %d, RST: %d            \n", 
                      _displays[i].config.cs_pin,
                      _displays[i].config.dc_pin,
                      _displays[i].config.rst_pin);
        
        if (_displays[i].currentApp) {
            Serial.printf("│   App: %s                      \n", 
                          _displays[i].currentApp->getName());
        } else {
            Serial.println("│   App: None                         │");
        }
        
        if (i < _displayCount - 1) {
            Serial.println("│                                     │");
        }
    }
    
    Serial.println("└─────────────────────────────────────┘");
}

// ========================================
// Private Methods
// ========================================

DisplayConfig DisplayManager::_getDefaultConfig(uint8_t displayId) {
    DisplayConfig config;
    config.id = displayId;
    config.width = 240;
    config.height = 320;
    
    switch (displayId) {
        case 0:  // Display 0 (original)
            config.cs_pin = 33;
            config.dc_pin = 15;
            config.rst_pin = 16;
            break;
            
        case 1:  // Display 1 (your new one)
            config.cs_pin = 34;
            config.dc_pin = 17;
            config.rst_pin = 18;
            break;
            
        case 2:  // Display 2 (future)
            config.cs_pin = 21;
            config.dc_pin = 47;
            config.rst_pin = 48;
            break;
    }
    
    return config;
}

bool DisplayManager::_initDisplay(uint8_t displayId, const DisplayConfig& config) {
    // Initialize GPIO pins
    pinMode(config.cs_pin, OUTPUT);
    pinMode(config.dc_pin, OUTPUT);
    pinMode(config.rst_pin, OUTPUT);
    
    digitalWrite(config.cs_pin, HIGH);  // Deselect
    digitalWrite(config.dc_pin, HIGH);
    
    // Hardware reset
    digitalWrite(config.rst_pin, HIGH);
    delay(10);
    digitalWrite(config.rst_pin, LOW);
    delay(20);
    digitalWrite(config.rst_pin, HIGH);
    delay(150);
    
    // Initialize SPI (shared bus, only do once)
    if (displayId == 0) {
        SPI.begin(36, -1, 37, config.cs_pin);  // SCLK=36, MOSI=37
        SPI.setFrequency(40000000);  // 40MHz
    }
    
    // ST7789 initialization sequence
    digitalWrite(config.cs_pin, LOW);  // Select this display
    
    // Software reset
    digitalWrite(config.dc_pin, LOW);  // Command mode
    SPI.transfer(0x01);  // SWRESET
    digitalWrite(config.cs_pin, HIGH);
    delay(150);
    
    digitalWrite(config.cs_pin, LOW);
    
    // Sleep out
    digitalWrite(config.dc_pin, LOW);
    SPI.transfer(0x11);  // SLPOUT
    digitalWrite(config.cs_pin, HIGH);
    delay(120);
    
    digitalWrite(config.cs_pin, LOW);
    
    // Color mode: 16-bit
    digitalWrite(config.dc_pin, LOW);
    SPI.transfer(0x3A);  // COLMOD
    digitalWrite(config.dc_pin, HIGH);
    SPI.transfer(0x55);  // 16-bit/pixel
    digitalWrite(config.cs_pin, HIGH);
    delay(10);
    
    digitalWrite(config.cs_pin, LOW);
    
    // Memory data access control
    digitalWrite(config.dc_pin, LOW);
    SPI.transfer(0x36);  // MADCTL
    digitalWrite(config.dc_pin, HIGH);
    SPI.transfer(0x00);  // Portrait, RGB
    digitalWrite(config.cs_pin, HIGH);
    delay(10);
    
    digitalWrite(config.cs_pin, LOW);
    
    // Normal display on
    digitalWrite(config.dc_pin, LOW);
    SPI.transfer(0x13);  // NORON
    digitalWrite(config.cs_pin, HIGH);
    delay(10);
    
    digitalWrite(config.cs_pin, LOW);
    
    // Display on
    digitalWrite(config.dc_pin, LOW);
    SPI.transfer(0x29);  // DISPON
    digitalWrite(config.cs_pin, HIGH);
    delay(100);
    
    // Create LVGL display for this display
    // (This will be implemented when integrating with LVGL)
    // For now, just mark as successful
    
    return true;
}

} // namespace Doki