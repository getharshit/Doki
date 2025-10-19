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

// Doki OS Configuration
#include "hardware_config.h"
#include "timing_constants.h"

// Doki OS Modules
#include "doki/storage_manager.h"
#include "doki/wifi_manager.h"
#include "doki/setup_portal.h"
#include "doki/qr_generator.h"
#include "doki/app_manager.h"
#include "doki/weather_service.h"
#include "doki/time_service.h"
#include "doki/simple_http_server.h"
#include "doki/filesystem_manager.h"
#include "doki/media_service.h"
#include "doki/media_cache.h"
#include "doki/lvgl_fs_driver.h"
#include "doki/state_persistence.h"
#include "doki/lvgl_manager.h"
#include "doki/js_engine.h"
#include "doki/js_app.h"

// WebSocket support (if enabled)
#define ENABLE_WEBSOCKET_SUPPORT  // Enable for C++ test
#ifdef ENABLE_WEBSOCKET_SUPPORT
    #include <WebSocketsClient.h>
#endif

// Import apps
#include "apps/clock_app/clock_app.h"
#include "apps/weather_app/weather_app.h"
#include "apps/sysinfo_app/sysinfo_app.h"
#include "apps/hello_app/hello_app.h"
#include "apps/goodbye_app/goodbye_app.h"
#include "apps/blank_app/blank_app.h"
#include "apps/image_preview/image_preview.h"
#include "apps/gif_player/gif_player.h"
#include "apps/sprite_player/sprite_player.h"
#include "apps/custom_js/custom_js_app.h"
#include "apps/benchmark/benchmark.h"

// ========================================
// Hardware Configuration (now in hardware_config.h)
// ========================================

// Using constants from hardware_config.h
#define DISP0_CS DISPLAY_0_CS_PIN
#define DISP0_DC DISPLAY_0_DC_PIN
#define DISP0_RST DISPLAY_0_RST_PIN

#define DISP1_CS DISPLAY_1_CS_PIN
#define DISP1_DC DISPLAY_1_DC_PIN
#define DISP1_RST DISPLAY_1_RST_PIN

#define TFT_SCLK SPI_SCLK_PIN
#define TFT_MOSI SPI_MOSI_PIN

#define TFT_WIDTH DISPLAY_WIDTH
#define TFT_HEIGHT DISPLAY_HEIGHT
// DISPLAY_COUNT already defined in hardware_config.h

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

// Unified display flush function (eliminates duplication)
void lvgl_flush_display_generic(lv_disp_drv_t* disp, const lv_area_t* area,
                                 lv_color_t* color_p, uint8_t cs_pin, uint8_t dc_pin) {
    uint32_t w = (area->x2 - area->x1 + 1);
    uint32_t h = (area->y2 - area->y1 + 1);
    setAddrWindow(cs_pin, dc_pin, area->x1, area->y1, area->x2, area->y2);
    digitalWrite(dc_pin, HIGH);
    digitalWrite(cs_pin, LOW);
    for (uint32_t i = 0; i < w * h; i++) {
        spi.transfer16(color_p[i].full);
    }
    digitalWrite(cs_pin, HIGH);
    lv_disp_flush_ready(disp);
}

// Display-specific wrappers (for LVGL callback compatibility)
void lvgl_flush_display0(lv_disp_drv_t* disp, const lv_area_t* area, lv_color_t* color_p) {
    lvgl_flush_display_generic(disp, area, color_p, DISP0_CS, DISP0_DC);
}

void lvgl_flush_display1(lv_disp_drv_t* disp, const lv_area_t* area, lv_color_t* color_p) {
    lvgl_flush_display_generic(disp, area, color_p, DISP1_CS, DISP1_DC);
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

// Old functions removed - now using AppManager

void enterNormalMode() {
    Serial.println("\n╔═══════════════════════════════════════════════════╗");
    Serial.println("║         NORMAL MODE - Loading Apps                ║");
    Serial.println("╚═══════════════════════════════════════════════════╝\n");

    // Initialize TimeService (centralized NTP singleton)
    Doki::TimeService::getInstance().begin();

    // Initialize weather service
    Doki::WeatherService::init(WEATHER_API_KEY);

    // Start HTTP server (no callbacks needed - uses AppManager directly)
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

    // Load default apps using AppManager
    Doki::AppManager::loadApp(0, "clock");
    Doki::AppManager::loadApp(1, "weather");

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

    // Step 1: Initialize Storage (NVS for WiFi credentials)
    Serial.println("[Main] Step 1/6: Initializing storage...");
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

    // Step 1.5: Initialize Filesystem (SPIFFS for media files)
    Serial.println("\n[Main] Step 1.5/6: Initializing filesystem...");
    if (!Doki::FilesystemManager::init(true)) {
        Serial.println("[Main] ✗ Filesystem initialization failed!");
        while (1) delay(1000);
    }

    // Initialize Media Service
    if (!Doki::MediaService::init()) {
        Serial.println("[Main] ✗ Media service initialization failed!");
        while (1) delay(1000);
    }

    // Initialize MediaCache (PSRAM-based caching)
    Serial.println("\n[Main] Step 1.6/6: Initializing MediaCache...");
    if (!Doki::MediaCache::init()) {
        Serial.println("[Main] ✗ MediaCache initialization failed!");
        while (1) delay(1000);
    }

    // Step 2: Initialize LVGL
    Serial.println("\n[Main] Step 2/5: Initializing LVGL...");
    lv_init();
    Serial.println("[Main] ✓ LVGL initialized");

    // Step 2.5: Initialize LVGL Thread Safety
    Serial.println("\n[Main] Step 2.5/7: Initializing LVGL mutex...");
    if (!Doki::LVGLManager::init()) {
        Serial.println("[Main] ✗ LVGL mutex initialization failed!");
        while (1) delay(1000);
    }

    // Register LVGL filesystem driver for SPIFFS
    if (!Doki::LvglFsDriver::init()) {
        Serial.println("[Main] ✗ LVGL filesystem driver initialization failed!");
        while (1) delay(1000);
    }

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
    Doki::LVGLManager::lock();
    showBootSplash(0);
    showBootSplash(1);
    lv_timer_handler();
    Doki::LVGLManager::unlock();
    delay(2000);

    // Step 3.5: Initialize StatePersistence
    Serial.println("\n[Main] Step 3.5/8: Initializing StatePersistence...");
    if (!Doki::StatePersistence::init()) {
        Serial.println("[Main] ✗ StatePersistence initialization failed!");
        while (1) delay(1000);
    }

    // Step 3.6: Initialize JavaScript Engine
    Serial.println("\n[Main] Step 3.6/8: Initializing JavaScript Engine...");
    if (!Doki::JSEngine::init()) {
        Serial.println("[Main] ⚠️  JavaScript support not available (continuing without it)");
    }

    // Step 3.7: Initialize AppManager
    Serial.println("\n[Main] Step 3.7/8: Initializing AppManager...");
    lv_disp_t* displayHandles[DISPLAY_COUNT];
    for (uint8_t i = 0; i < DISPLAY_COUNT; i++) {
        displayHandles[i] = displays[i].disp;
    }
    if (!Doki::AppManager::init(DISPLAY_COUNT, displayHandles)) {
        Serial.println("[Main] ✗ AppManager initialization failed!");
        while (1) delay(1000);
    }

    // Register all apps
    Serial.println("[Main] Registering apps...");
    Doki::AppManager::registerApp("clock", "Clock", []() -> Doki::DokiApp* { return new ClockApp(); }, "Displays current time");
    Doki::AppManager::registerApp("weather", "Weather", []() -> Doki::DokiApp* { return new WeatherApp(); }, "Shows weather information");
    Doki::AppManager::registerApp("sysinfo", "System Info", []() -> Doki::DokiApp* { return new SysInfoApp(); }, "System diagnostics");
    Doki::AppManager::registerApp("hello", "Hello", []() -> Doki::DokiApp* { return new HelloApp(); }, "Hello World app");
    Doki::AppManager::registerApp("goodbye", "Goodbye", []() -> Doki::DokiApp* { return new GoodbyeApp(); }, "Goodbye app");
    Doki::AppManager::registerApp("blank", "Blank", []() -> Doki::DokiApp* { return new BlankApp(); }, "Blank screen");
    Doki::AppManager::registerApp("image", "Image Viewer", []() -> Doki::DokiApp* { return new ImagePreviewApp(); }, "Display images");
    Doki::AppManager::registerApp("gif", "GIF Player", []() -> Doki::DokiApp* { return new GifPlayerApp(); }, "Play animated GIFs");
    Doki::AppManager::registerApp("sprite_player", "Sprite Player", []() -> Doki::DokiApp* { return new SpritePlayerApp(); }, "Play uploaded sprite animations");
    Doki::AppManager::registerApp("benchmark", "Benchmark", []() -> Doki::DokiApp* { return new Doki::BenchmarkApp(); }, "Display performance testing");

    // Custom JavaScript app (loads code based on which display it's running on)
    Doki::AppManager::registerApp("custom", "Custom JS", []() -> Doki::DokiApp* { return new CustomJSApp(); }, "User-programmable JavaScript app");

    // Advanced Demo - Showcases all JavaScript features (animations, MQTT, WebSocket, HTTP, multi-display)
    Doki::AppManager::registerApp("advanced_demo", "Advanced Demo",
        []() -> Doki::DokiApp* {
            return new Doki::JSApp("advanced_demo", "Advanced Demo", "/apps/advanced_demo.js");
        },
        "Comprehensive demo of animations, MQTT, WebSocket, and multi-display features");

    // WebSocket Test - Diagnostic app for WebSocket connectivity testing
    Doki::AppManager::registerApp("websocket_test", "WebSocket Test",
        []() -> Doki::DokiApp* {
            return new Doki::JSApp("websocket_test", "WebSocket Test", "/apps/websocket_test.js");
        },
        "WebSocket diagnostic and testing tool");

    // Animation Chain - Sequential animation playback demo
    Doki::AppManager::registerApp("animation_chain", "Animation Chain",
        []() -> Doki::DokiApp* {
            return new Doki::JSApp("animation_chain", "Animation Chain", "/apps/animation_chain.js");
        },
        "Demo of sequential animation playback with smooth transitions");

    // Stress Test - Animation system stress testing
    Doki::AppManager::registerApp("stress_test", "Stress Test",
        []() -> Doki::DokiApp* {
            return new Doki::JSApp("stress_test", "Stress Test", "/apps/stress_test.js");
        },
        "Comprehensive stress testing for animation system (memory, performance, concurrency)");

    // Cloud Weather - Animated cloud weather display
    Doki::AppManager::registerApp("cloud_weather", "Cloud Weather",
        []() -> Doki::DokiApp* {
            return new Doki::JSApp("cloud_weather", "Cloud Weather", "/apps/cloud_weather.js");
        },
        "Animated cloud weather visualization (30 frames, 200x150)");

    Doki::AppManager::printStatus();

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

        // ========== DIAGNOSTIC TESTS (moved to separate file in Phase 2) ==========
        // TODO: Move to test/network_diagnostics.cpp
        #ifdef DEBUG_WEBSOCKET_DIAGNOSTICS
        // ========== TCP CLIENT TEST (DIAGNOSTIC) ==========
        Serial.println("\n========== TCP Client Test ==========");
        Serial.println("[TCP Test] Testing basic WiFiClient (same class WebSocket uses)");
        Serial.printf("[TCP Test] Free heap: %d bytes\n", ESP.getFreeHeap());

        WiFiClient tcpClient;
        const char* testHost = "echo.websocket.org";
        const uint16_t testPort = 80;

        Serial.printf("[TCP Test] Connecting to %s:%d...\n", testHost, testPort);

        unsigned long tcpStartTime = millis();
        bool tcpConnected = tcpClient.connect(testHost, testPort);
        unsigned long tcpConnectTime = millis() - tcpStartTime;

        Serial.printf("[TCP Test] Connection attempt took %lu ms\n", tcpConnectTime);
        Serial.printf("[TCP Test] Result: %s\n", tcpConnected ? "SUCCESS" : "FAILED");

        if (tcpConnected) {
            Serial.println("[TCP Test] ✓ TCP connection successful!");
            Serial.printf("[TCP Test] Connected to: %s\n", tcpClient.remoteIP().toString().c_str());
            Serial.printf("[TCP Test] Local port: %d\n", tcpClient.localPort());

            // Send simple HTTP request
            Serial.println("[TCP Test] Sending HTTP GET request...");
            tcpClient.println("GET / HTTP/1.1");
            tcpClient.println("Host: echo.websocket.org");
            tcpClient.println("Connection: close");
            tcpClient.println();

            // Wait for response
            Serial.println("[TCP Test] Waiting for response...");
            unsigned long responseStart = millis();
            while (!tcpClient.available() && millis() - responseStart < 5000) {
                delay(10);
            }

            if (tcpClient.available()) {
                Serial.println("[TCP Test] ✓ Response received:");
                String line = tcpClient.readStringUntil('\n');
                Serial.printf("[TCP Test]   %s\n", line.c_str());
            } else {
                Serial.println("[TCP Test] ✗ No response received");
            }

            tcpClient.stop();
            Serial.println("[TCP Test] Connection closed");

            Serial.println("\n[TCP Test] ✓ VERDICT: WiFiClient TCP works perfectly!");
            Serial.println("[TCP Test] → Issue must be in WebSocket library layer");
        } else {
            Serial.println("\n[TCP Test] ✗ VERDICT: Basic TCP connection FAILED!");
            Serial.println("[TCP Test] → This explains why WebSocket fails");
            Serial.printf("[TCP Test] → WiFi status: %d\n", WiFi.status());
            Serial.printf("[TCP Test] → DNS IP: %s\n", WiFi.dnsIP().toString().c_str());
        }
        Serial.println("==========================================\n");
        // ========== END TCP TEST ==========

        // ========== MANUAL WEBSOCKET HANDSHAKE TEST ==========
        Serial.println("\n========== Manual WebSocket Handshake Test ==========");
        Serial.println("[WS Manual] Attempting WebSocket handshake without library");

        WiFiClient wsManualClient;
        Serial.printf("[WS Manual] Connecting to %s:%d...\n", testHost, testPort);

        if (wsManualClient.connect(testHost, testPort)) {
            Serial.println("[WS Manual] ✓ TCP connected");

            // Send WebSocket upgrade request
            Serial.println("[WS Manual] Sending WebSocket upgrade request...");
            wsManualClient.print("GET / HTTP/1.1\r\n");
            wsManualClient.print("Host: echo.websocket.org\r\n");
            wsManualClient.print("Upgrade: websocket\r\n");
            wsManualClient.print("Connection: Upgrade\r\n");
            wsManualClient.print("Sec-WebSocket-Key: dGhlIHNhbXBsZSBub25jZQ==\r\n");
            wsManualClient.print("Sec-WebSocket-Version: 13\r\n");
            wsManualClient.print("\r\n");

            // Wait for response
            Serial.println("[WS Manual] Waiting for upgrade response...");
            unsigned long wsResponseStart = millis();
            while (!wsManualClient.available() && millis() - wsResponseStart < 5000) {
                delay(10);
            }

            if (wsManualClient.available()) {
                Serial.println("[WS Manual] ✓ Response received:");
                while (wsManualClient.available()) {
                    String line = wsManualClient.readStringUntil('\n');
                    Serial.printf("[WS Manual]   %s\n", line.c_str());
                    if (line.length() <= 1) break;  // Empty line = end of headers
                }

                Serial.println("\n[WS Manual] ✓ VERDICT: WebSocket handshake works manually!");
                Serial.println("[WS Manual] → The WebSocket PROTOCOL works fine");
                Serial.println("[WS Manual] → Problem is in the Links2004 library implementation");
            } else {
                Serial.println("[WS Manual] ✗ No response to upgrade request");
            }

            wsManualClient.stop();
        } else {
            Serial.println("[WS Manual] ✗ TCP connection failed");
        }
        Serial.println("=================================================\n");
        // ========== END MANUAL WEBSOCKET TEST ==========

        // ========== WEBSOCKET C++ DIRECT TEST ==========
        Serial.println("\n========== WebSocket C++ Direct Test (Links2004 Library) ==========");
        #ifdef ENABLE_WEBSOCKET_SUPPORT
            Serial.println("[WS C++ Test] WebSocket support is ENABLED (Links2004)");
            Serial.printf("[WS C++ Test] Free heap before test: %d bytes\n", ESP.getFreeHeap());

            Serial.println("[WS C++ Test] Creating WebSocket client...");
            WebSocketsClient* testWsClient = new WebSocketsClient();
            Serial.printf("[WS C++ Test] Client created at: %p\n", (void*)testWsClient);
            Serial.printf("[WS C++ Test] Free heap after creation: %d bytes\n", ESP.getFreeHeap());

            // Links2004 library uses event callbacks
            Serial.println("[WS C++ Test] Setting up event handler...");
            testWsClient->onEvent([](WStype_t type, uint8_t* payload, size_t length) {
                switch(type) {
                    case WStype_DISCONNECTED:
                        Serial.println("[WS C++ Test] Disconnected");
                        break;
                    case WStype_CONNECTED:
                        Serial.printf("[WS C++ Test] ✓✓✓ CONNECTED to: %s\n", payload);
                        break;
                    case WStype_TEXT:
                        Serial.printf("[WS C++ Test] ✓✓✓ MESSAGE RECEIVED: %s\n", payload);
                        break;
                    case WStype_ERROR:
                        Serial.printf("[WS C++ Test] ✗ Error: %s\n", payload);
                        break;
                    default:
                        Serial.printf("[WS C++ Test] Event type: %d\n", type);
                        break;
                }
            });

            // Test servers with host/port format (Links2004 API)
            // NOTE: Many WebSocket servers now require WSS (secure) on port 443
            struct TestServer {
                const char* host;
                uint16_t port;
                const char* path;
                bool useSSL;
            };

            TestServer testServers[] = {
                {"socketsbay.com", 443, "/wss/v2/1/demo/", true},  // WSS - Testing focused
                {"echo-websocket.fly.dev", 443, "/", true},  // WSS - Modern echo server
                {"ws.ifelse.io", 443, "/", true}  // WSS - Community recommended
            };

            bool testSuccess = false;
            for (int i = 0; i < 3; i++) {
                Serial.printf("\n[WS C++ Test] Attempt #%d - Server: %s:%d%s (%s)\n",
                    i+1, testServers[i].host, testServers[i].port, testServers[i].path,
                    testServers[i].useSSL ? "WSS/SSL" : "WS/plain");

                // Links2004 API: beginSSL() for secure, begin() for plain
                if (testServers[i].useSSL) {
                    Serial.println("[WS C++ Test] Using beginSSL() for secure connection");
                    testWsClient->beginSSL(testServers[i].host, testServers[i].port, testServers[i].path);
                } else {
                    Serial.println("[WS C++ Test] Using begin() for plain connection");
                    testWsClient->begin(testServers[i].host, testServers[i].port, testServers[i].path);
                }

                Serial.println("[WS C++ Test] Waiting for connection (5 sec)...");

                // Loop to handle events
                unsigned long startTime = millis();
                bool connected = false;
                while (millis() - startTime < 5000) {  // 5 second timeout
                    testWsClient->loop();

                    // Check if connected (Links2004 doesn't have direct isConnected method)
                    // We rely on event callback
                    delay(100);
                }

                // Try to send a message
                Serial.println("[WS C++ Test] Sending test message...");
                bool sendSuccess = testWsClient->sendTXT("Hello from C++ Links2004 test!");

                if (sendSuccess) {
                    Serial.println("[WS C++ Test] ✓ Message sent successfully!");
                    testSuccess = true;

                    // Wait for echo response
                    Serial.println("[WS C++ Test] Waiting for echo (3 sec)...");
                    startTime = millis();
                    while (millis() - startTime < 3000) {
                        testWsClient->loop();
                        delay(100);
                    }

                    break;
                } else {
                    Serial.println("[WS C++ Test] ✗ Send failed - likely not connected");
                }

                testWsClient->disconnect();
                delay(500);
            }

            delete testWsClient;

            if (testSuccess) {
                Serial.println("\n[WS C++ Test] ✓ VERDICT: Links2004 WebSocket library works in C++!");
                Serial.println("[WS C++ Test] → Now need to update JS bindings to use this library");
            } else {
                Serial.println("\n[WS C++ Test] ✗ VERDICT: WebSocket still not working");
                Serial.println("[WS C++ Test] → May need different library or configuration");
            }

            Serial.printf("[WS C++ Test] Final heap: %d bytes\n", ESP.getFreeHeap());
        #else
            Serial.println("[WS C++ Test] WebSocket support NOT enabled");
            Serial.println("[WS C++ Test] Enable #define ENABLE_WEBSOCKET_SUPPORT in main.cpp");
        #endif
        Serial.println("===============================================\n");
        // ========== END WEBSOCKET TEST ==========
        #endif // DEBUG_WEBSOCKET_DIAGNOSTICS
        // ========== END DIAGNOSTIC TESTS ==========

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
        // Normal Mode: Update all apps via AppManager
        Doki::AppManager::update();

        // Handle WiFi reconnection
        Doki::WiFiManager::handleReconnection();
    }

    // Always update LVGL (protected by mutex)
    Doki::LVGLManager::lock();
    lv_timer_handler();
    Doki::LVGLManager::unlock();

    // OPTIMIZATION: Removed delay(5) to maximize FPS
    // Run loop as fast as possible for best animation performance
}
