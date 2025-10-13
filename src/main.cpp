/**
 * @file main.cpp
 * @brief Test program for Task Scheduler + Memory Manager + Event System + App Base Class
 */

#include <Arduino.h>
#include <lvgl.h>
#include "st7789_driver.h"
#include "doki/app_base.h"
#include "doki/event_system.h"
#include "doki/memory_manager.h"
#include "doki/task_scheduler.h"
#include "apps/hello_app/hello_app.h"

// LVGL display buffers
#define DISP_BUF_SIZE (TFT_WIDTH * 40)
static lv_disp_draw_buf_t draw_buf;
static lv_color_t* buf1;
static lv_color_t* buf2;
static lv_disp_drv_t disp_drv;

// Test app instance
HelloApp* app = nullptr;

// Test task counters
int task1Counter = 0;
int task2Counter = 0;

// ========================================
// Test Task Functions
// ========================================

void fastTask(int taskId) {
    Serial.printf("[Task %d] Fast task started (runs every 500ms)\n", taskId);
    
    while (true) {
        task1Counter++;
        Serial.printf("[Task %d] Fast tick #%d\n", taskId, task1Counter);
        vTaskDelay(pdMS_TO_TICKS(500));  // 500ms delay
        
        // Stop after 5 iterations for testing
        if (task1Counter >= 5) {
            Serial.printf("[Task %d] Fast task completing after 5 iterations\n", taskId);
            break;
        }
    }
    
    // Task will auto-cleanup when function returns
}

void slowTask(int taskId) {
    Serial.printf("[Task %d] Slow task started (runs every 2 seconds)\n", taskId);
    
    while (true) {
        task2Counter++;
        Serial.printf("[Task %d] Slow tick #%d\n", taskId, task2Counter);
        vTaskDelay(pdMS_TO_TICKS(2000));  // 2 second delay
        
        // Run forever (will be stopped manually)
    }
}

void setup() {
    Serial.begin(115200);
    delay(1000);
    
    Serial.println("\n╔═══════════════════════════════════╗");
    Serial.println("║   Doki OS - Task Scheduler Test ║");
    Serial.println("║   Milestone 3.1 - Step 5         ║");
    Serial.println("╚═══════════════════════════════════╝\n");
    
    // ========================================
    // 1. Initialize Display & LVGL
    // ========================================
    Serial.println("→ Initializing display driver...");
    display_driver.init();
    Serial.println("  ✓ Display ready");
    
    Serial.println("\n→ Initializing LVGL...");
    lv_init();
    
    buf1 = (lv_color_t*)heap_caps_malloc(DISP_BUF_SIZE * sizeof(lv_color_t), MALLOC_CAP_SPIRAM);
    buf2 = (lv_color_t*)heap_caps_malloc(DISP_BUF_SIZE * sizeof(lv_color_t), MALLOC_CAP_SPIRAM);
    
    if (!buf1 || !buf2) {
        Serial.println("  ✗ ERROR: Failed to allocate display buffers!");
        while(1) delay(1000);
    }
    
    lv_disp_draw_buf_init(&draw_buf, buf1, buf2, DISP_BUF_SIZE);
    
    lv_disp_drv_init(&disp_drv);
    disp_drv.hor_res = TFT_WIDTH;
    disp_drv.ver_res = TFT_HEIGHT;
    disp_drv.flush_cb = lvgl_flush_cb;
    disp_drv.draw_buf = &draw_buf;
    lv_disp_drv_register(&disp_drv);
    
    Serial.println("  ✓ LVGL initialized\n");
    
    // ========================================
    // 2. Test Task Scheduler
    // ========================================
    Serial.println("╔═══════════════════════════════════╗");
    Serial.println("║   Testing Task Scheduler         ║");
    Serial.println("╚═══════════════════════════════════╝\n");
    
    // TEST 1: Create a fast task
    Serial.println("TEST 1: Creating fast task (500ms interval)...");
    int task1 = Doki::TaskScheduler::createTask(
        "test_app",                           // App ID
        "FastTask",                           // Task name
        fastTask,                             // Task function
        4096,                                 // Stack size
        Doki::TaskPriority::NORMAL           // Priority
    );
    Serial.printf("  Task ID: %d\n\n", task1);
    delay(1000);
    
    // TEST 2: Create a slow task with higher priority
    Serial.println("TEST 2: Creating slow task (2s interval, higher priority)...");
    int task2 = Doki::TaskScheduler::createTask(
        "test_app",
        "SlowTask",
        slowTask,
        4096,
        Doki::TaskPriority::HIGHER
    );
    Serial.printf("  Task ID: %d\n\n", task2);
    delay(1000);
    
    // TEST 3: Check active task count
    Serial.println("TEST 3: Checking active task count...");
    int activeCount = Doki::TaskScheduler::getActiveTaskCount();
    Serial.printf("  Active tasks: %d (expected: 2)\n", activeCount);
    Serial.printf("  Result: %s\n\n", activeCount == 2 ? "✓ PASS" : "✗ FAIL");
    delay(1000);
    
    // TEST 4: Print task report
    Serial.println("TEST 4: Printing task report...");
    Doki::TaskScheduler::printTaskReport();
    Serial.println();
    delay(1000);
    
    // TEST 5: Get app tasks
    Serial.println("TEST 5: Getting tasks for 'test_app'...");
    std::vector<int> appTasks = Doki::TaskScheduler::getAppTasks("test_app");
    Serial.printf("  Found %d task(s)\n", appTasks.size());
    for (int id : appTasks) {
        Doki::TaskInfo info = Doki::TaskScheduler::getTaskInfo(id);
        Serial.printf("    - Task ID %d: %s\n", id, info.name.c_str());
    }
    Serial.printf("  Result: %s\n\n", appTasks.size() == 2 ? "✓ PASS" : "✗ FAIL");
    
    // Wait for fast task to complete (should stop after 5 iterations = ~2.5 seconds)
    Serial.println("Waiting for fast task to complete (~3 seconds)...\n");
    delay(3500);
    
    // TEST 6: Verify fast task stopped itself
    Serial.println("TEST 6: Checking if fast task auto-stopped...");
    activeCount = Doki::TaskScheduler::getActiveTaskCount();
    Serial.printf("  Active tasks: %d (expected: 1, only slow task)\n", activeCount);
    Serial.printf("  Fast task counter: %d (should be 5)\n", task1Counter);
    Serial.printf("  Result: %s\n\n", (activeCount == 1 && task1Counter == 5) ? "✓ PASS" : "✗ FAIL");
    
    // TEST 7: Manually stop the slow task
    Serial.println("TEST 7: Manually stopping slow task...");
    bool stopped = Doki::TaskScheduler::stopTask(task2);
    Serial.printf("  Stop result: %s\n", stopped ? "success" : "failed");
    activeCount = Doki::TaskScheduler::getActiveTaskCount();
    Serial.printf("  Active tasks: %d (expected: 0)\n", activeCount);
    Serial.printf("  Result: %s\n\n", (stopped && activeCount == 0) ? "✓ PASS" : "✗ FAIL");
    
    // TEST 8: Create HelloApp with a background task
    Serial.println("TEST 8: Creating HelloApp with background task...");
    app = new HelloApp();
    app->onCreate();
    app->_setState(Doki::AppState::CREATED);
    app->onStart();
    app->_setState(Doki::AppState::STARTED);
    app->_markStarted();
    
    // Create a background task for HelloApp
    int appTask = Doki::TaskScheduler::createTask(
        "hello",
        "HelloBgTask",
        [](int id) {
            Serial.printf("[Task %d] HelloApp background task running\n", id);
            for (int i = 0; i < 3; i++) {
                Serial.printf("[Task %d] Background tick #%d\n", id, i+1);
                vTaskDelay(pdMS_TO_TICKS(1000));
            }
            Serial.printf("[Task %d] HelloApp background task completed\n", id);
        },
        4096,
        Doki::TaskPriority::NORMAL
    );
    Serial.printf("  Created background task ID: %d\n\n", appTask);
    
    delay(4000);  // Wait for HelloApp task to complete
    
    // TEST 9: Stop all app tasks
    Serial.println("TEST 9: Stopping all tasks for 'hello' app...");
    int stoppedCount = Doki::TaskScheduler::stopAppTasks("hello");
    Serial.printf("  Stopped %d task(s)\n", stoppedCount);
    Serial.printf("  Result: %s\n\n", stoppedCount >= 0 ? "✓ PASS" : "✗ FAIL");
    
    Serial.println("╔═══════════════════════════════════╗");
    Serial.println("║   ✓ Task Scheduler Tests Done!  ║");
    Serial.println("║   HelloApp is running...         ║");
    Serial.println("╚═══════════════════════════════════╝\n");
    
    // Print final system status
    Serial.println("Final System Status:");
    Doki::MemoryManager::printSystemReport();
    Doki::TaskScheduler::printTaskReport();
    Serial.println();
    
    Serial.println("--- App Updates Below ---\n");
}

void loop() {
    // Update LVGL graphics
    lv_timer_handler();
    
    // Update app if running
    if (app && app->isRunning()) {
        app->onUpdate();
    }
    
    delay(5);
}