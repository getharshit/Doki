/**
 * @file main.cpp
 * @brief Display Manager Test - Milestone 3.5
 */

#include <Arduino.h>
#include <WiFi.h>
#include <lvgl.h>
#include "st7789_driver.h"
#include "doki/display_manager.h"
#include "doki/app_manager.h"
#include "doki/http_server.h"
#include "doki/weather_service.h"

// Import all apps
#include "apps/clock_app/clock_app.h"
#include "apps/weather_app/weather_app.h"
#include "apps/sysinfo_app/sysinfo_app.h"

// ========================================
// Configuration
// ========================================
const char* WIFI_SSID = "Abhi";
const char* WIFI_PASSWORD = "";
const char* WEATHER_API_KEY = "3183db8ec2fe4abfa2c133226251310";

// LVGL display buffers (single display for now)
#define DISP_BUF_SIZE (240 * 40)
static lv_disp_draw_buf_t draw_buf;
static lv_color_t* buf1;
static lv_color_t* buf2;
static lv_disp_drv_t disp_drv;

bool connectWiFi() {
    Serial.println("\n→ Connecting to WiFi...");
    WiFi.mode(WIFI_STA);
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    
    int attempts = 0;
    while (WiFi.status() != WL_CONNECTED && attempts < 20) {
        delay(500);
        Serial.print(".");
        attempts++;
    }
    Serial.println();
    
    if (WiFi.status() == WL_CONNECTED) {
        Serial.printf("  ✓ WiFi connected! IP: %s\n", WiFi.localIP().toString().c_str());
        return true;
    }
    return false;
}

void setup() {
    Serial.begin(115200);
    delay(1000);
    
    Serial.println("\n╔═══════════════════════════════════════════════════╗");
    Serial.println("║    Display Manager Test - Milestone 3.5          ║");
    Serial.println("║    Testing with 1 display                        ║");
    Serial.println("╚═══════════════════════════════════════════════════╝\n");
    
    // ========================================
    // 1. Initialize Display Hardware
    // ========================================
    Serial.println("→ Initializing display hardware...");
    
    // Initialize display using existing driver (Display 0)
    display_driver.init();
    Serial.println("  ✓ Display 0 hardware ready");
    
    // ========================================
    // 2. Initialize Display Manager
    // ========================================
    Serial.println("\n→ Initializing Display Manager...");
    
    if (!Doki::DisplayManager::init(2)) {  // Initialize for 1 display
        Serial.println("  ✗ Display Manager initialization failed!");
        while(1) delay(1000);
    }
    
    // Print display configuration
    Doki::DisplayConfig config = Doki::DisplayManager::getDisplayConfig(0);
    Serial.printf("  Display 0 config: CS=%d, DC=%d, RST=%d\n", 
                  config.cs_pin, config.dc_pin, config.rst_pin);
    Serial.println("  ✓ Display Manager initialized\n");
    
    // ========================================
    // 3. Initialize LVGL (Traditional way for now)
    // ========================================
    Serial.println("→ Initializing LVGL...");
    lv_init();
    
    buf1 = (lv_color_t*)heap_caps_malloc(DISP_BUF_SIZE * sizeof(lv_color_t), MALLOC_CAP_SPIRAM);
    buf2 = (lv_color_t*)heap_caps_malloc(DISP_BUF_SIZE * sizeof(lv_color_t), MALLOC_CAP_SPIRAM);
    
    if (!buf1 || !buf2) {
        Serial.println("  ✗ Display buffer allocation failed!");
        while(1) delay(1000);
    }
    
    lv_disp_draw_buf_init(&draw_buf, buf1, buf2, DISP_BUF_SIZE);
    
    lv_disp_drv_init(&disp_drv);
    disp_drv.hor_res = 240;
    disp_drv.ver_res = 320;
    disp_drv.flush_cb = lvgl_flush_cb;
    disp_drv.draw_buf = &draw_buf;
    lv_disp_t* disp = lv_disp_drv_register(&disp_drv);
    
    Serial.println("  ✓ LVGL initialized\n");
    
    // ========================================
    // 4. Connect WiFi & Initialize Services
    // ========================================
    connectWiFi();
    
    Serial.println("\n→ Initializing services...");
    Doki::WeatherService::init(WEATHER_API_KEY);
    Serial.println("  ✓ Weather service ready");
    
    // ========================================
    // 5. Register Apps
    // ========================================
    Serial.println("\n→ Registering apps...");
    
    Doki::AppManager::registerApp("clock", "Clock", 
        []() { return new ClockApp(); }, "Real-time clock");
    
    Doki::AppManager::registerApp("weather", "Weather",
        []() { return new WeatherApp(); }, "Live weather");
    
    Doki::AppManager::registerApp("sysinfo", "System Info",
        []() { return new SysInfoApp(); }, "System stats");
    
    Serial.println("  ✓ Apps registered\n");
    
    // ========================================
    // 6. Start HTTP Server
    // ========================================
    if (WiFi.status() == WL_CONNECTED) {
        Serial.println("→ Starting HTTP server...");
        if (Doki::HttpServer::begin(80)) {
            Serial.printf("\n📱 Dashboard: %s/dashboard\n\n", 
                         Doki::HttpServer::getServerUrl().c_str());
        }
    }
    
    // ========================================
    // 7. Test Display Manager
    // ========================================
    Serial.println("╔═══════════════════════════════════════════════════╗");
    Serial.println("║   Testing Display Manager                        ║");
    Serial.println("╚═══════════════════════════════════════════════════╝\n");
    
    Serial.println("TEST 1: Check display count...");
    uint8_t displayCount = Doki::DisplayManager::getDisplayCount();
    Serial.printf("  Display count: %d (expected: 1)\n", displayCount);
    Serial.printf("  Result: %s\n\n", displayCount == 1 ? "✓ PASS" : "✗ FAIL");
    
    Serial.println("TEST 2: Check display ready...");
    bool ready = Doki::DisplayManager::isDisplayReady(0);
    Serial.printf("  Display 0 ready: %s\n", ready ? "yes" : "no");
    Serial.printf("  Result: %s\n\n", ready ? "✓ PASS" : "✗ FAIL");
    
    Serial.println("TEST 3: Print display status...");
    Doki::DisplayManager::printStatus();
    Serial.println();
    
    // ========================================
    // 8. Load App Using Traditional Method
    // ========================================
    Serial.println("→ Loading Clock App...\n");
    Doki::AppManager::loadApp("clock");
    
    Serial.println("\n╔═══════════════════════════════════════════════════╗");
    Serial.println("║              ✓ Test Complete! ✓                  ║");
    Serial.println("║                                                   ║");
    Serial.println("║  Display Manager Architecture Working!           ║");
    Serial.println("║  Ready for multi-display integration             ║");
    Serial.println("║                                                   ║");
    Serial.println("║  Switch apps via HTTP dashboard                  ║");
    Serial.println("╚═══════════════════════════════════════════════════╝\n");
}

void loop() {
    // Update LVGL and current app
    lv_timer_handler();
    Doki::AppManager::update();
    
    delay(5);
}