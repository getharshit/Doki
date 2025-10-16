/**
 * @file display_manager.cpp
 * @brief Implementation of Display Manager for Doki OS
 */

#include "doki/display_manager.h"
#include "doki/qr_generator.h"
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

// ========================================
// Setup Screen Methods
// ========================================

bool DisplayManager::showSetupScreen(uint8_t displayId,
                                      const String& ssid,
                                      const String& password,
                                      const String& url) {
    if (displayId >= _displayCount || !_displays[displayId].initialized) {
        Serial.printf("[DisplayManager] Error: Display %d not available\n", displayId);
        return false;
    }

    Serial.printf("[DisplayManager] Showing setup screen on Display %d\n", displayId);

    // Set LVGL to use this display
    lv_disp_set_default(_displays[displayId].lvgl_display);

    // Get the screen object
    lv_obj_t* screen = lv_disp_get_scr_act(_displays[displayId].lvgl_display);

    // Use QR Generator to create complete setup screen
    String setupURL = url.isEmpty() ? ("http://192.168.4.1/setup") : url;
    QRGenerator::createSetupScreen(screen, ssid, password, setupURL);

    Serial.printf("[DisplayManager] ✓ Setup screen displayed on Display %d\n", displayId);
    return true;
}

bool DisplayManager::showStatusMessage(uint8_t displayId,
                                        const String& message,
                                        bool isError) {
    if (displayId >= _displayCount || !_displays[displayId].initialized) {
        return false;
    }

    Serial.printf("[DisplayManager] Display %d: %s\n", displayId, message.c_str());

    // Set LVGL to use this display
    lv_disp_set_default(_displays[displayId].lvgl_display);

    // Get the screen object
    lv_obj_t* screen = lv_disp_get_scr_act(_displays[displayId].lvgl_display);

    // Clear screen
    lv_obj_clean(screen);
    lv_obj_set_style_bg_color(screen, lv_color_hex(0x000000), 0);

    // Create message label
    lv_obj_t* msgLabel = lv_label_create(screen);
    lv_label_set_text(msgLabel, message.c_str());
    lv_label_set_long_mode(msgLabel, LV_LABEL_LONG_WRAP);
    lv_obj_set_width(msgLabel, 200);
    lv_obj_center(msgLabel);
    lv_obj_set_style_text_align(msgLabel, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_style_text_font(msgLabel, &lv_font_montserrat_16, 0);

    // Color based on message type
    if (isError) {
        lv_obj_set_style_text_color(msgLabel, lv_color_hex(0xFF4444), 0);
    } else {
        lv_obj_set_style_text_color(msgLabel, lv_color_hex(0x10b981), 0);
    }

    return true;
}

bool DisplayManager::clearSetupScreen(uint8_t displayId) {
    if (displayId >= _displayCount || !_displays[displayId].initialized) {
        return false;
    }

    Serial.printf("[DisplayManager] Clearing setup screen on Display %d\n", displayId);

    // Set LVGL to use this display
    lv_disp_set_default(_displays[displayId].lvgl_display);

    // Get the screen object
    lv_obj_t* screen = lv_disp_get_scr_act(_displays[displayId].lvgl_display);

    // Clear screen
    lv_obj_clean(screen);
    lv_obj_set_style_bg_color(screen, lv_color_hex(0x000000), 0);

    return true;
}

bool DisplayManager::showBootSplash(uint8_t displayId) {
    if (displayId >= _displayCount || !_displays[displayId].initialized) {
        return false;
    }

    Serial.printf("[DisplayManager] Showing boot splash on Display %d\n", displayId);

    // Set LVGL to use this display
    lv_disp_set_default(_displays[displayId].lvgl_display);

    // Get the screen object
    lv_obj_t* screen = lv_disp_get_scr_act(_displays[displayId].lvgl_display);

    // Clear screen
    lv_obj_clean(screen);
    lv_obj_set_style_bg_color(screen, lv_color_hex(0x000000), 0);

    // Doki OS Title
    lv_obj_t* title = lv_label_create(screen);
    lv_label_set_text(title, "Doki OS");
    lv_obj_align(title, LV_ALIGN_CENTER, 0, -40);
    lv_obj_set_style_text_font(title, &lv_font_montserrat_32, 0);
    lv_obj_set_style_text_color(title, lv_color_hex(0x667eea), 0);

    // Version
    lv_obj_t* version = lv_label_create(screen);
    lv_label_set_text(version, "v0.2.0");
    lv_obj_align(version, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_style_text_font(version, &lv_font_montserrat_16, 0);
    lv_obj_set_style_text_color(version, lv_color_hex(0x888888), 0);

    // Loading message
    lv_obj_t* loading = lv_label_create(screen);
    lv_label_set_text(loading, "Initializing...");
    lv_obj_align(loading, LV_ALIGN_CENTER, 0, 40);
    lv_obj_set_style_text_font(loading, &lv_font_montserrat_12, 0);
    lv_obj_set_style_text_color(loading, lv_color_hex(0x10b981), 0);

    return true;
}

} // namespace Doki