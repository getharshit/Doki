/**
 * @file main.cpp
 * @brief Test program for DokiApp base class
 */

#include <Arduino.h>
#include <lvgl.h>
#include "st7789_driver.h"
#include "apps/hello_app/hello_app.h"

// LVGL display buffers
#define DISP_BUF_SIZE (TFT_WIDTH * 40)
static lv_disp_draw_buf_t draw_buf;
static lv_color_t* buf1;
static lv_color_t* buf2;
static lv_disp_drv_t disp_drv;

// Test app instance
HelloApp* app = nullptr;

void setup() {
    Serial.begin(115200);
    delay(1000);
    
    Serial.println("\n╔═══════════════════════════════════╗");
    Serial.println("║   Doki OS - App Base Test       ║");
    Serial.println("║   Milestone 3.1 - Step 2         ║");
    Serial.println("╚═══════════════════════════════════╝\n");
    
    // Initialize Display Driver
    Serial.println("→ Initializing display driver...");
    display_driver.init();
    Serial.println("  ✓ Display ready");
    
    // Initialize LVGL
    Serial.println("\n→ Initializing LVGL...");
    lv_init();
    
    // Allocate display buffers in PSRAM
    buf1 = (lv_color_t*)heap_caps_malloc(DISP_BUF_SIZE * sizeof(lv_color_t), MALLOC_CAP_SPIRAM);
    buf2 = (lv_color_t*)heap_caps_malloc(DISP_BUF_SIZE * sizeof(lv_color_t), MALLOC_CAP_SPIRAM);
    
    if (!buf1 || !buf2) {
        Serial.println("  ✗ ERROR: Failed to allocate display buffers!");
        while(1) delay(1000);
    }
    
    lv_disp_draw_buf_init(&draw_buf, buf1, buf2, DISP_BUF_SIZE);
    
    // Register display driver
    lv_disp_drv_init(&disp_drv);
    disp_drv.hor_res = TFT_WIDTH;
    disp_drv.ver_res = TFT_HEIGHT;
    disp_drv.flush_cb = lvgl_flush_cb;
    disp_drv.draw_buf = &draw_buf;
    lv_disp_drv_register(&disp_drv);
    
    Serial.println("  ✓ LVGL initialized");
    Serial.printf("  ✓ Using PSRAM for display buffers (%d bytes)\n", DISP_BUF_SIZE * 2 * sizeof(lv_color_t));
    
    // Test App Lifecycle
    Serial.println("\n╔═══════════════════════════════════╗");
    Serial.println("║   Testing App Lifecycle          ║");
    Serial.println("╚═══════════════════════════════════╝\n");
    
    // TEST 1: Create app
    Serial.println("TEST 1: Creating HelloApp...");
    app = new HelloApp();
    Serial.println("  ✓ App object created\n");
    delay(500);
    
    // TEST 2: Initialize app (onCreate)
    Serial.println("TEST 2: Calling onCreate()...");
    app->onCreate();
    app->_setState(Doki::AppState::CREATED);
    Serial.println("  ✓ App UI created\n");
    delay(500);
    
    // TEST 3: Start app (onStart)
    Serial.println("TEST 3: Calling onStart()...");
    app->onStart();
    app->_setState(Doki::AppState::STARTED);
    app->_markStarted();
    Serial.println("  ✓ App started\n");
    
    Serial.println("╔═══════════════════════════════════╗");
    Serial.println("║   ✓ App is Running!              ║");
    Serial.println("║   Watch the display update...    ║");
    Serial.println("╚═══════════════════════════════════╝\n");
    
    // Print system info
    Serial.printf("Free Heap: %d bytes\n", ESP.getFreeHeap());
    Serial.printf("Free PSRAM: %d bytes\n", ESP.getFreePsram());
    Serial.printf("App State: %s\n", app->isRunning() ? "RUNNING" : "STOPPED");
    Serial.println("\n--- Live Updates Below ---\n");
}

void loop() {
    // Update LVGL graphics
    lv_timer_handler();
    
    // Update app if it's running
    if (app && app->isRunning()) {
        app->onUpdate();
    }
    
    delay(5);
}