/**
 * @file main.cpp (REFACTORED - HYBRID APPROACH)
 * @brief Doki OS - Clean Modular Architecture with Working Display Init
 *
 * This version combines:
 * - Original working LVGL display initialization
 * - New modular architecture (StorageManager, WiFiManager, SetupPortal, QRGenerator)
 * - Clean separation of concerns
 */

#include <Arduino.h>
#include <WiFi.h>
#include <SPI.h>
#include <lvgl.h>

// Doki OS Modules
#include "doki/storage_manager.h"
#include "doki/wifi_manager.h"
#include "doki/setup_portal.h"
#include "doki/qr_generator.h"
#include "doki/app_manager.h"
#include "doki/weather_service.h"
#include "doki/simple_http_server.h"

// Import apps
#include "apps/clock_app/clock_app.h"
#include "apps/weather_app/weather_app.h"
#include "apps/sysinfo_app/sysinfo_app.h"
#include "apps/hello_app/hello_app.h"
#include "apps/goodbye_app/goodbye_app.h"
#include "apps/blank_app/blank_app.h"

// ========================================
// Hardware Configuration
// ========================================

// Display Pins (from original working configuration)
#define DISP0_CS 33
#define DISP0_DC 15
#define DISP0_RST 16

#define DISP1_CS 34
#define DISP1_DC 17
#define DISP1_RST 18

// SPI Pins
#define TFT_SCLK 36
#define TFT_MOSI 37

// Display Configuration
#define TFT_WIDTH 240
#define TFT_HEIGHT 320
#define DISPLAY_COUNT 2

// ST7789 Commands
#define ST7789_SWRESET 0x01
#define ST7789_SLPOUT 0x11
#define ST7789_COLMOD 0x3A
#define ST7789_MADCTL 0x36
#define ST7789_CASET 0x2A
#define ST7789_RASET 0x2B
#define ST7789_RAMWR 0x2C
#define ST7789_INVON 0x21
#define ST7789_NORON 0x13
#define ST7789_DISPON 0x29

// WiFi Configuration
#define AP_SSID "DokiOS-Setup"
#define AP_PASSWORD "doki1234"

// Weather API
#define WEATHER_API_KEY "3183db8ec2fe4abfa2c133226251310"

// ========================================
// Display Structure
// ========================================

struct Display {
    uint8_t id;
    uint8_t cs_pin;
    uint8_t dc_pin;
    uint8_t rst_pin;

    lv_color_t* buf1;
    lv_color_t* buf2;
    lv_disp_draw_buf_t draw_buf;
    lv_disp_drv_t disp_drv;
    lv_disp_t* disp;

    Doki::DokiApp* app;
    String appId;
    uint32_t appStartTime;
};

Display displays[DISPLAY_COUNT];
SPIClass spi(HSPI);

// ========================================
// Setup Mode State
// ========================================

bool setupMode = false;
uint32_t setupModeStartTime = 0;
const uint32_t SETUP_MODE_TIMEOUT = 300000; // 5 minutes

// ========================================
// ST7789 Low-Level Functions
// ========================================

void writeCommand(uint8_t cs, uint8_t dc, uint8_t cmd) {
    digitalWrite(dc, LOW);
    digitalWrite(cs, LOW);
    spi.transfer(cmd);
    digitalWrite(cs, HIGH);
}

void writeData(uint8_t cs, uint8_t dc, uint8_t data) {
    digitalWrite(dc, HIGH);
    digitalWrite(cs, LOW);
    spi.transfer(data);
    digitalWrite(cs, HIGH);
}

void setAddrWindow(uint8_t cs, uint8_t dc, uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1) {
    writeCommand(cs, dc, ST7789_CASET);
    writeData(cs, dc, x0 >> 8);
    writeData(cs, dc, x0 & 0xFF);
    writeData(cs, dc, x1 >> 8);
    writeData(cs, dc, x1 & 0xFF);

    writeCommand(cs, dc, ST7789_RASET);
    writeData(cs, dc, y0 >> 8);
    writeData(cs, dc, y0 & 0xFF);
    writeData(cs, dc, y1 >> 8);
    writeData(cs, dc, y1 & 0xFF);

    writeCommand(cs, dc, ST7789_RAMWR);
}

void initDisplayHardware(uint8_t cs, uint8_t dc, uint8_t rst) {
    pinMode(cs, OUTPUT);
    pinMode(dc, OUTPUT);
    pinMode(rst, OUTPUT);

    digitalWrite(cs, HIGH);
    digitalWrite(rst, HIGH);
    delay(10);
    digitalWrite(rst, LOW);
    delay(20);
    digitalWrite(rst, HIGH);
    delay(150);

    writeCommand(cs, dc, ST7789_SWRESET);
    delay(150);
    writeCommand(cs, dc, ST7789_SLPOUT);
    delay(10);
    writeCommand(cs, dc, ST7789_COLMOD);
    writeData(cs, dc, 0x55);
    writeCommand(cs, dc, ST7789_MADCTL);
    writeData(cs, dc, 0x00);
    writeCommand(cs, dc, ST7789_INVON);
    delay(10);
    writeCommand(cs, dc, ST7789_NORON);
    delay(10);
    writeCommand(cs, dc, ST7789_DISPON);
    delay(100);
}

// ========================================
// LVGL Flush Callbacks
// ========================================

void lvgl_flush_display0(lv_disp_drv_t* disp, const lv_area_t* area, lv_color_t* color_p) {
    uint32_t w = (area->x2 - area->x1 + 1);
    uint32_t h = (area->y2 - area->y1 + 1);
    setAddrWindow(DISP0_CS, DISP0_DC, area->x1, area->y1, area->x2, area->y2);
    digitalWrite(DISP0_DC, HIGH);
    digitalWrite(DISP0_CS, LOW);
    for (uint32_t i = 0; i < w * h; i++) {
        spi.transfer16(color_p[i].full);
    }
    digitalWrite(DISP0_CS, HIGH);
    lv_disp_flush_ready(disp);
}

void lvgl_flush_display1(lv_disp_drv_t* disp, const lv_area_t* area, lv_color_t* color_p) {
    uint32_t w = (area->x2 - area->x1 + 1);
    uint32_t h = (area->y2 - area->y1 + 1);
    setAddrWindow(DISP1_CS, DISP1_DC, area->x1, area->y1, area->x2, area->y2);
    digitalWrite(DISP1_DC, HIGH);
    digitalWrite(DISP1_CS, LOW);
    for (uint32_t i = 0; i < w * h; i++) {
        spi.transfer16(color_p[i].full);
    }
    digitalWrite(DISP1_CS, HIGH);
    lv_disp_flush_ready(disp);
}

// ========================================
// Display Initialization (Original Working Code)
// ========================================

bool initDisplay(uint8_t id, uint8_t cs, uint8_t dc, uint8_t rst) {
    Display* d = &displays[id];
    d->id = id;
    d->cs_pin = cs;
    d->dc_pin = dc;
    d->rst_pin = rst;
    d->app = nullptr;
    d->appId = "";
    d->appStartTime = 0;

    // Initialize ST7789 hardware
    initDisplayHardware(cs, dc, rst);

    // Allocate LVGL buffers in PSRAM
    size_t buf_size = TFT_WIDTH * 40;
    d->buf1 = (lv_color_t*)heap_caps_malloc(buf_size * sizeof(lv_color_t), MALLOC_CAP_SPIRAM);
    d->buf2 = (lv_color_t*)heap_caps_malloc(buf_size * sizeof(lv_color_t), MALLOC_CAP_SPIRAM);

    if (!d->buf1 || !d->buf2) {
        Serial.printf("[Main] ✗ Failed to allocate buffers for Display %d\n", id);
        return false;
    }

    // Initialize LVGL display buffer
    lv_disp_draw_buf_init(&d->draw_buf, d->buf1, d->buf2, buf_size);

    // Initialize LVGL display driver
    lv_disp_drv_init(&d->disp_drv);
    d->disp_drv.hor_res = TFT_WIDTH;
    d->disp_drv.ver_res = TFT_HEIGHT;
    d->disp_drv.flush_cb = (id == 0) ? lvgl_flush_display0 : lvgl_flush_display1;
    d->disp_drv.draw_buf = &d->draw_buf;

    // Register LVGL display
    d->disp = lv_disp_drv_register(&d->disp_drv);

    if (d->disp == nullptr) {
        Serial.printf("[Main] ✗ Failed to register LVGL display %d\n", id);
        return false;
    }

    Serial.printf("[Main] ✓ Display %d initialized successfully\n", id);
    return true;
}

// ========================================
// Boot Splash
// ========================================

void showBootSplash(uint8_t displayId) {
    if (displayId >= DISPLAY_COUNT) return;

    Display* d = &displays[displayId];
    lv_disp_set_default(d->disp);
    lv_obj_t* screen = lv_disp_get_scr_act(d->disp);
    lv_obj_clean(screen);

    lv_obj_set_style_bg_color(screen, lv_color_hex(0x667EEA), 0);

    lv_obj_t* label = lv_label_create(screen);
    lv_label_set_text(label, "Doki OS\nv0.2.0");
    lv_obj_set_style_text_color(label, lv_color_white(), 0);
    lv_obj_set_style_text_align(label, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_center(label);
}

// ========================================
// Setup Mode Functions
// ========================================

void enterSetupMode() {
    Serial.println("\n╔═══════════════════════════════════════════════════╗");
    Serial.println("║           ENTERING SETUP MODE                      ║");
    Serial.println("╚═══════════════════════════════════════════════════╝\n");

    setupMode = true;
    setupModeStartTime = millis();

    // Start Access Point
    if (!Doki::WiFiManager::startAccessPoint(AP_SSID, AP_PASSWORD)) {
        Serial.println("[Main] ✗ Failed to start Access Point");
        return;
    }

    // Start Setup Portal
    if (!Doki::SetupPortal::begin()) {
        Serial.println("[Main] ✗ Failed to start Setup Portal");
        return;
    }

    // Get setup URL
    String setupURL = "http://" + WiFi.softAPIP().toString() + "/setup";

    // Display 1: Step 1 - Connect to WiFi AP
    Serial.println("[Main] Setting up Display 1 - Step 1...");
    lv_disp_set_default(displays[1].disp);
    lv_obj_t* screen1 = lv_disp_get_scr_act(displays[1].disp);
    lv_obj_clean(screen1);
    lv_obj_set_style_bg_color(screen1, lv_color_hex(0x1F2937), 0);

    // Main heading: DOKI OS
    lv_obj_t* heading = lv_label_create(screen1);
    lv_label_set_text(heading, "DOKI OS");
    lv_obj_align(heading, LV_ALIGN_TOP_MID, 0, 20);
    lv_obj_set_style_text_color(heading, lv_color_hex(0x667EEA), 0);
    lv_obj_set_style_text_font(heading, &lv_font_montserrat_32, 0);

    // Subheading: Step 1
    lv_obj_t* subheading = lv_label_create(screen1);
    lv_label_set_text(subheading, "Step 1: Connect to WiFi");
    lv_obj_align(subheading, LV_ALIGN_TOP_MID, 0, 65);
    lv_obj_set_style_text_color(subheading, lv_color_hex(0x10B981), 0);
    lv_obj_set_style_text_font(subheading, &lv_font_montserrat_20, 0);

    // WiFi Details
    lv_obj_t* details = lv_label_create(screen1);
    lv_label_set_text(details, "Network Name:\nDokiOS-Setup\n\nPassword:\ndoki1234");
    lv_obj_align(details, LV_ALIGN_CENTER, 0, 20);
    lv_obj_set_style_text_color(details, lv_color_white(), 0);
    lv_obj_set_style_text_align(details, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_style_text_font(details, &lv_font_montserrat_18, 0);

    // Footer instruction
    lv_obj_t* footer1 = lv_label_create(screen1);
    lv_label_set_text(footer1, "See other display for Step 2");
    lv_obj_align(footer1, LV_ALIGN_BOTTOM_MID, 0, -15);
    lv_obj_set_style_text_color(footer1, lv_color_hex(0x888888), 0);
    lv_obj_set_style_text_font(footer1, &lv_font_montserrat_12, 0);

    // Display 0: Step 2 - QR Code
    Serial.println("[Main] Setting up Display 0 - Step 2 with QR code...");
    lv_disp_set_default(displays[0].disp);
    lv_obj_t* screen0 = lv_disp_get_scr_act(displays[0].disp);
    lv_obj_clean(screen0);
    lv_obj_set_style_bg_color(screen0, lv_color_hex(0x000000), 0);

    // Step 2 heading
    lv_obj_t* step2heading = lv_label_create(screen0);
    lv_label_set_text(step2heading, "Step 2");
    lv_obj_align(step2heading, LV_ALIGN_TOP_MID, 0, 15);
    lv_obj_set_style_text_color(step2heading, lv_color_hex(0x667EEA), 0);
    lv_obj_set_style_text_font(step2heading, &lv_font_montserrat_24, 0);

    // QR Code for setup URL (larger size with scale=4)
    lv_obj_t* qr = Doki::QRGenerator::displayURLQR(screen0, setupURL, -1, 60, 4);
    if (qr) {
        lv_obj_align(qr, LV_ALIGN_CENTER, 0, 0);
    }

    // Instructions below QR
    lv_obj_t* instructions = lv_label_create(screen0);
    lv_label_set_text(instructions, "Scan to visit\nsetup page");
    lv_obj_align(instructions, LV_ALIGN_BOTTOM_MID, 0, -20);
    lv_obj_set_style_text_color(instructions, lv_color_white(), 0);
    lv_obj_set_style_text_align(instructions, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_style_text_font(instructions, &lv_font_montserrat_14, 0);

    // Force render both displays
    Serial.println("[Main] Rendering displays...");
    lv_timer_handler();
    delay(100);
    lv_timer_handler();

    Serial.printf("\n[Main] Connect to: %s (Password: %s)\n", AP_SSID, AP_PASSWORD);
    Serial.printf("[Main] Then visit: %s\n\n", setupURL.c_str());
}

void exitSetupMode() {
    Serial.println("[Main] Exiting setup mode...");
    setupMode = false;
    Doki::SetupPortal::stop();
}

// ========================================
// Normal Mode Functions
// ========================================

Doki::DokiApp* createApp(const String& appId) {
    if (appId == "clock") return new ClockApp();
    if (appId == "weather") return new WeatherApp();
    if (appId == "sysinfo") return new SysInfoApp();
    if (appId == "hello") return new HelloApp();
    if (appId == "goodbye") return new GoodbyeApp();
    if (appId == "blank") return new BlankApp();
    return nullptr;
}

bool loadAppOnDisplay(uint8_t displayId, const String& appId) {
    if (displayId >= DISPLAY_COUNT) return false;

    Display* d = &displays[displayId];

    // Set display context and clear screen FIRST
    lv_disp_set_default(d->disp);
    lv_obj_t* screen = lv_disp_get_scr_act(d->disp);
    lv_obj_clean(screen);  // Clear screen before destroying app to prevent dangling pointers

    // Now destroy old app after screen is cleaned
    if (d->app) {
        d->app->onDestroy();
        delete d->app;
        d->app = nullptr;
    }

    // Create and load new app
    d->app = createApp(appId);
    if (!d->app) {
        Serial.printf("[Main] ✗ Failed to create app '%s'\n", appId.c_str());
        return false;
    }

    // Initialize app on clean screen
    d->app->onCreate();
    d->app->onStart();

    d->appId = appId;
    d->appStartTime = millis();

    Serial.printf("[Main] ✓ Loaded app '%s' on Display %d\n", appId.c_str(), displayId);
    return true;
}


// ========================================
// HTTP Server Callbacks
// ========================================

void getDisplayStatus(uint8_t displayId, String& appId, uint32_t& uptime) {
    if (displayId >= DISPLAY_COUNT) {
        appId = "";
        uptime = 0;
        return;
    }
    appId = displays[displayId].appId;
    uptime = displays[displayId].app ? (millis() - displays[displayId].appStartTime) / 1000 : 0;
}

void enterNormalMode() {
    Serial.println("\n╔═══════════════════════════════════════════════════╗");
    Serial.println("║         NORMAL MODE - Loading Apps                ║");
    Serial.println("╚═══════════════════════════════════════════════════╝\n");

    // Initialize weather service
    Doki::WeatherService::init(WEATHER_API_KEY);

    // Set HTTP server callbacks
    Doki::SimpleHttpServer::setLoadAppCallback(loadAppOnDisplay);
    Doki::SimpleHttpServer::setStatusCallback(getDisplayStatus);

    // Start HTTP server
    if (Doki::SimpleHttpServer::begin(80)) {
        String ip = WiFi.localIP().toString();
        Serial.println("\n╔═══════════════════════════════════════════════════╗");
        Serial.println("║            System Connected                        ║");
        Serial.println("╚═══════════════════════════════════════════════════╝");
        Serial.printf("\n📱 Dashboard: http://%s\n", ip.c_str());
        Serial.printf("🌐 IP Address: %s\n\n", ip.c_str());
    } else {
        Serial.println("[Main] ✗ Failed to start HTTP server");
    }

    // Load default apps
    loadAppOnDisplay(0, "clock");
    loadAppOnDisplay(1, "weather");

    Serial.println("\n[Main] ✓ System ready!\n");
}

// ========================================
// Setup
// ========================================

void setup() {
    Serial.begin(115200);
    delay(1000);

    Serial.println("\n╔═══════════════════════════════════════════════════╗");
    Serial.println("║         Doki OS - Refactored Architecture         ║");
    Serial.println("║                  Version 0.2.0                     ║");
    Serial.println("╚═══════════════════════════════════════════════════╝\n");

    // Step 1: Initialize Storage
    Serial.println("[Main] Step 1/5: Initializing storage...");
    if (!Doki::StorageManager::init()) {
        Serial.println("[Main] ✗ Storage initialization failed!");
        while (1) delay(1000);
    }

    // Check for saved WiFi credentials
    String savedSSID, savedPassword;
    bool hasCredentials = Doki::StorageManager::loadWiFiCredentials(savedSSID, savedPassword);
    if (hasCredentials) {
        Serial.printf("[Main] Found saved WiFi: %s\n", savedSSID.c_str());
    } else {
        Serial.println("[Main] No saved WiFi credentials");
    }

    // Step 2: Initialize LVGL
    Serial.println("\n[Main] Step 2/5: Initializing LVGL...");
    lv_init();
    Serial.println("[Main] ✓ LVGL initialized");

    // Step 3: Initialize Displays (Using Original Working Code)
    Serial.println("\n[Main] Step 3/5: Initializing displays...");

    spi.begin(TFT_SCLK, -1, TFT_MOSI);
    spi.setFrequency(40000000);

    if (!initDisplay(0, DISP0_CS, DISP0_DC, DISP0_RST) ||
        !initDisplay(1, DISP1_CS, DISP1_DC, DISP1_RST)) {
        Serial.println("[Main] ✗ Display initialization failed!");
        while (1) delay(1000);
    }

    // Show boot splash
    Serial.println("[Main] Showing boot splash...");
    showBootSplash(0);
    showBootSplash(1);
    lv_timer_handler();
    delay(2000);

    // Step 4: Initialize WiFi Manager
    Serial.println("\n[Main] Step 4/5: Initializing WiFi...");
    if (!Doki::WiFiManager::init()) {
        Serial.println("[Main] ✗ WiFi Manager initialization failed!");
        while (1) delay(1000);
    }

    // Step 5: Connect to WiFi or Enter Setup Mode
    Serial.println("\n[Main] Step 5/5: Connecting to WiFi...");

    bool connected = false;
    if (hasCredentials) {
        Serial.println("[Main] Attempting to connect with saved credentials...");
        connected = Doki::WiFiManager::connectToWiFi(savedSSID, savedPassword, 10000);
    }

    if (!connected) {
        Serial.println("[Main] Auto-connect failed or no credentials");
        enterSetupMode();
    } else {
        Serial.printf("[Main] ✓ Connected to WiFi: %s\n", WiFi.localIP().toString().c_str());
        enterNormalMode();
    }

    Serial.println("\n[Main] ✓ Setup complete!\n");
}

// ========================================
// Loop
// ========================================

void loop() {
    if (setupMode) {
        // Setup Mode: Handle captive portal
        Doki::SetupPortal::update();

        // Check for timeout
        if (millis() - setupModeStartTime > SETUP_MODE_TIMEOUT) {
            Serial.println("[Main] Setup mode timeout - restarting...");
            ESP.restart();
        }
    } else {
        // Normal Mode: Update apps
        for (uint8_t i = 0; i < DISPLAY_COUNT; i++) {
            if (displays[i].app) {
                lv_disp_set_default(displays[i].disp);
                displays[i].app->onUpdate();
            }
        }

        // Handle WiFi reconnection
        Doki::WiFiManager::handleReconnection();
    }

    // Always update LVGL
    lv_timer_handler();
    delay(5);
}
