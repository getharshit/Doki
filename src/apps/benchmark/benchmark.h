/**
 * @file benchmark.h
 * @brief Display Performance Benchmarking App for Doki OS
 *
 * Measures various display performance metrics:
 * - Screen fill speed
 * - Pixel write speed
 * - Rectangle/line/circle rendering
 * - Text rendering speed
 * - LVGL refresh performance
 */

#ifndef DOKI_APP_BENCHMARK_H
#define DOKI_APP_BENCHMARK_H

#include "doki/app_base.h"
#include <lvgl.h>

namespace Doki {

class BenchmarkApp : public DokiApp {
public:
    BenchmarkApp() : DokiApp("benchmark", "Benchmark"),
                     _testStage(0),
                     _lastUpdate(0),
                     _resultsLabel(nullptr),
                     _progressLabel(nullptr),
                     _running(false) {}

    ~BenchmarkApp() override = default;

    void onCreate() override {
        Serial.println("[BenchmarkApp] Creating benchmark app...");

        lv_obj_t* screen = getScreen();

        // Set dark background
        lv_obj_set_style_bg_color(screen, lv_color_hex(0x1a1a1a), 0);

        // Title
        lv_obj_t* title = lv_label_create(screen);
        lv_label_set_text(title, "Display Benchmark");
        lv_obj_set_style_text_color(title, lv_color_hex(0x60a5fa), 0);
        lv_obj_set_style_text_font(title, &lv_font_montserrat_20, 0);
        lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 10);

        // Display info
        uint8_t displayId = getDisplayId();
        lv_obj_t* infoLabel = lv_label_create(screen);
        lv_label_set_text_fmt(infoLabel, "Display %d - 240x320 ST7789", displayId);
        lv_obj_set_style_text_color(infoLabel, lv_color_hex(0x94a3b8), 0);
        lv_obj_set_style_text_font(infoLabel, &lv_font_montserrat_14, 0);
        lv_obj_align(infoLabel, LV_ALIGN_TOP_MID, 0, 40);

        // Progress label
        _progressLabel = lv_label_create(screen);
        lv_label_set_text(_progressLabel, "Ready to start...");
        lv_obj_set_style_text_color(_progressLabel, lv_color_hex(0x10b981), 0);
        lv_obj_set_style_text_font(_progressLabel, &lv_font_montserrat_14, 0);
        lv_obj_align(_progressLabel, LV_ALIGN_TOP_MID, 0, 70);

        // Results label (multiline)
        _resultsLabel = lv_label_create(screen);
        lv_label_set_text(_resultsLabel, "Initializing benchmark...");
        lv_obj_set_style_text_color(_resultsLabel, lv_color_hex(0xe5e7eb), 0);
        lv_obj_set_style_text_font(_resultsLabel, &lv_font_montserrat_12, 0);
        lv_obj_set_width(_resultsLabel, 220);
        lv_label_set_long_mode(_resultsLabel, LV_LABEL_LONG_WRAP);
        lv_obj_align(_resultsLabel, LV_ALIGN_TOP_LEFT, 10, 100);
    }

    void onStart() override {
        Serial.println("[BenchmarkApp] Benchmark app started!");
        // Auto-start benchmark
        startBenchmark();
    }

    void onUpdate() override {
        if (!_running) return;

        uint32_t now = millis();

        // Run tests sequentially with delays to show progress
        if (now - _lastUpdate < 100) return;  // 100ms between stages
        _lastUpdate = now;

        switch (_testStage) {
            case 0:
                runFillScreenTest();
                break;
            case 1:
                runPixelWriteTest();
                break;
            case 2:
                runRectangleTest();
                break;
            case 3:
                runLineTest();
                break;
            case 4:
                runCircleTest();
                break;
            case 5:
                runTextRenderTest();
                break;
            case 6:
                runPartialUpdateTest();
                break;
            case 7:
                showFinalResults();
                _running = false;
                break;
        }

        _testStage++;
    }

    void onDestroy() override {
        Serial.println("[BenchmarkApp] Destroying benchmark app");
        _resultsLabel = nullptr;
        _progressLabel = nullptr;
    }

private:
    struct BenchmarkResults {
        float fillScreenFPS;
        float pixelWriteKPS;      // Thousand pixels per second
        float rectangleFPS;
        float lineFPS;
        float circleFPS;
        float textCharPerSec;
        float partialUpdateFPS;
    } _results;

    uint8_t _testStage;
    uint32_t _lastUpdate;
    lv_obj_t* _resultsLabel;
    lv_obj_t* _progressLabel;
    bool _running;

    void startBenchmark() {
        Serial.println("[BenchmarkApp] Starting benchmark...");
        _running = true;
        _testStage = 0;
        _lastUpdate = millis();
        memset(&_results, 0, sizeof(_results));
        lv_label_set_text(_resultsLabel, "Running tests...");
        lv_label_set_text(_progressLabel, "Starting...");
    }

    void runFillScreenTest() {
        lv_label_set_text(_progressLabel, "Test 1/7: Screen Fill Speed");
        Serial.println("[Benchmark] Test 1: Fill Screen");

        const int iterations = 20;
        uint32_t start = millis();

        lv_obj_t* screen = getScreen();
        for (int i = 0; i < iterations; i++) {
            uint16_t color = 0x0000 + (i * 0x0841);  // Varying colors
            lv_obj_set_style_bg_color(screen, lv_color_hex(color), 0);
            lv_refr_now(NULL);  // Force immediate refresh
        }

        uint32_t elapsed = millis() - start;
        _results.fillScreenFPS = (float)iterations / (elapsed / 1000.0f);

        Serial.printf("[Benchmark] Fill Screen: %.2f FPS (%lu ms for %d fills)\n",
                     _results.fillScreenFPS, elapsed, iterations);
    }

    void runPixelWriteTest() {
        lv_label_set_text(_progressLabel, "Test 2/7: Pixel Write Speed");
        Serial.println("[Benchmark] Test 2: Pixel Write");

        lv_obj_t* screen = getScreen();
        lv_obj_set_style_bg_color(screen, lv_color_hex(0x000000), 0);

        const int pixelCount = 5000;
        uint32_t start = millis();

        // Draw random pixels using small rectangles
        for (int i = 0; i < pixelCount; i++) {
            int x = random(0, 240);
            int y = random(0, 320);
            uint16_t color = random(0, 0xFFFF);

            lv_obj_t* pixel = lv_obj_create(screen);
            lv_obj_set_size(pixel, 1, 1);
            lv_obj_set_pos(pixel, x, y);
            lv_obj_set_style_bg_color(pixel, lv_color_hex(color), 0);
            lv_obj_set_style_border_width(pixel, 0, 0);
            lv_obj_clear_flag(pixel, LV_OBJ_FLAG_SCROLLABLE);
        }

        lv_refr_now(NULL);
        uint32_t elapsed = millis() - start;
        _results.pixelWriteKPS = (float)pixelCount / elapsed;

        Serial.printf("[Benchmark] Pixel Write: %.2f K pixels/sec (%lu ms for %d pixels)\n",
                     _results.pixelWriteKPS, elapsed, pixelCount);

        // Clear screen
        lv_obj_clean(screen);
    }

    void runRectangleTest() {
        lv_label_set_text(_progressLabel, "Test 3/7: Rectangle Draw");
        Serial.println("[Benchmark] Test 3: Rectangle Draw");

        lv_obj_t* screen = getScreen();
        const int iterations = 100;
        uint32_t start = millis();

        for (int i = 0; i < iterations; i++) {
            int x = random(0, 200);
            int y = random(0, 280);
            int w = random(10, 40);
            int h = random(10, 40);
            uint16_t color = random(0, 0xFFFF);

            lv_obj_t* rect = lv_obj_create(screen);
            lv_obj_set_size(rect, w, h);
            lv_obj_set_pos(rect, x, y);
            lv_obj_set_style_bg_color(rect, lv_color_hex(color), 0);
            lv_obj_set_style_border_width(rect, 0, 0);
            lv_obj_clear_flag(rect, LV_OBJ_FLAG_SCROLLABLE);
        }

        lv_refr_now(NULL);
        uint32_t elapsed = millis() - start;
        _results.rectangleFPS = (float)iterations / (elapsed / 1000.0f);

        Serial.printf("[Benchmark] Rectangles: %.2f rects/sec (%lu ms for %d rects)\n",
                     _results.rectangleFPS, elapsed, iterations);

        lv_obj_clean(screen);
    }

    void runLineTest() {
        lv_label_set_text(_progressLabel, "Test 4/7: Line Draw");
        Serial.println("[Benchmark] Test 4: Line Draw");

        lv_obj_t* screen = getScreen();
        const int iterations = 100;
        uint32_t start = millis();

        for (int i = 0; i < iterations; i++) {
            static lv_point_t points[2];
            points[0].x = random(0, 240);
            points[0].y = random(0, 320);
            points[1].x = random(0, 240);
            points[1].y = random(0, 320);

            lv_obj_t* line = lv_line_create(screen);
            lv_line_set_points(line, points, 2);
            lv_obj_set_style_line_color(line, lv_color_hex(random(0, 0xFFFF)), 0);
            lv_obj_set_style_line_width(line, 2, 0);
        }

        lv_refr_now(NULL);
        uint32_t elapsed = millis() - start;
        _results.lineFPS = (float)iterations / (elapsed / 1000.0f);

        Serial.printf("[Benchmark] Lines: %.2f lines/sec (%lu ms for %d lines)\n",
                     _results.lineFPS, elapsed, iterations);

        lv_obj_clean(screen);
    }

    void runCircleTest() {
        lv_label_set_text(_progressLabel, "Test 5/7: Circle Draw");
        Serial.println("[Benchmark] Test 5: Circle Draw");

        lv_obj_t* screen = getScreen();
        const int iterations = 50;
        uint32_t start = millis();

        for (int i = 0; i < iterations; i++) {
            int x = random(20, 220);
            int y = random(20, 300);
            int radius = random(5, 20);
            uint16_t color = random(0, 0xFFFF);

            lv_obj_t* circle = lv_obj_create(screen);
            lv_obj_set_size(circle, radius * 2, radius * 2);
            lv_obj_set_pos(circle, x - radius, y - radius);
            lv_obj_set_style_radius(circle, radius, 0);
            lv_obj_set_style_bg_color(circle, lv_color_hex(color), 0);
            lv_obj_set_style_border_width(circle, 0, 0);
            lv_obj_clear_flag(circle, LV_OBJ_FLAG_SCROLLABLE);
        }

        lv_refr_now(NULL);
        uint32_t elapsed = millis() - start;
        _results.circleFPS = (float)iterations / (elapsed / 1000.0f);

        Serial.printf("[Benchmark] Circles: %.2f circles/sec (%lu ms for %d circles)\n",
                     _results.circleFPS, elapsed, iterations);

        lv_obj_clean(screen);
    }

    void runTextRenderTest() {
        lv_label_set_text(_progressLabel, "Test 6/7: Text Rendering");
        Serial.println("[Benchmark] Test 6: Text Rendering");

        lv_obj_t* screen = getScreen();
        const int iterations = 50;
        const char* testText = "Benchmark Test 123";
        const int charCount = strlen(testText);

        uint32_t start = millis();

        for (int i = 0; i < iterations; i++) {
            lv_obj_t* label = lv_label_create(screen);
            lv_label_set_text(label, testText);
            lv_obj_set_pos(label, random(0, 150), random(0, 280));
            lv_obj_set_style_text_color(label, lv_color_hex(random(0, 0xFFFF)), 0);
        }

        lv_refr_now(NULL);
        uint32_t elapsed = millis() - start;
        _results.textCharPerSec = (float)(iterations * charCount) / (elapsed / 1000.0f);

        Serial.printf("[Benchmark] Text: %.2f chars/sec (%lu ms for %d labels)\n",
                     _results.textCharPerSec, elapsed, iterations);

        lv_obj_clean(screen);
    }

    void runPartialUpdateTest() {
        lv_label_set_text(_progressLabel, "Test 7/7: Partial Updates");
        Serial.println("[Benchmark] Test 7: Partial Updates");

        lv_obj_t* screen = getScreen();

        // Create a small object to update repeatedly
        lv_obj_t* rect = lv_obj_create(screen);
        lv_obj_set_size(rect, 50, 50);
        lv_obj_set_style_border_width(rect, 0, 0);
        lv_obj_clear_flag(rect, LV_OBJ_FLAG_SCROLLABLE);

        const int iterations = 100;
        uint32_t start = millis();

        for (int i = 0; i < iterations; i++) {
            lv_obj_set_pos(rect, random(0, 190), random(0, 270));
            lv_obj_set_style_bg_color(rect, lv_color_hex(random(0, 0xFFFF)), 0);
            lv_refr_now(NULL);
        }

        uint32_t elapsed = millis() - start;
        _results.partialUpdateFPS = (float)iterations / (elapsed / 1000.0f);

        Serial.printf("[Benchmark] Partial Updates: %.2f FPS (%lu ms for %d updates)\n",
                     _results.partialUpdateFPS, elapsed, iterations);

        lv_obj_clean(screen);
    }

    void showFinalResults() {
        lv_label_set_text(_progressLabel, "Complete!");
        Serial.println("[Benchmark] === Final Results ===");

        // Format results string
        char resultsText[512];
        snprintf(resultsText, sizeof(resultsText),
            "BENCHMARK RESULTS\n\n"
            "Screen Fill:     %.1f FPS\n"
            "Pixel Write:     %.1f kpx/s\n"
            "Rectangles:      %.1f /s\n"
            "Lines:           %.1f /s\n"
            "Circles:         %.1f /s\n"
            "Text:            %.0f chr/s\n"
            "Partial Update:  %.1f FPS\n\n"
            "Reload app to test again",
            _results.fillScreenFPS,
            _results.pixelWriteKPS,
            _results.rectangleFPS,
            _results.lineFPS,
            _results.circleFPS,
            _results.textCharPerSec,
            _results.partialUpdateFPS
        );

        lv_label_set_text(_resultsLabel, resultsText);

        // Also print to Serial
        Serial.println("[Benchmark] === Results Summary ===");
        Serial.printf("Fill Screen:     %.2f FPS\n", _results.fillScreenFPS);
        Serial.printf("Pixel Write:     %.2f K pixels/sec\n", _results.pixelWriteKPS);
        Serial.printf("Rectangles:      %.2f rects/sec\n", _results.rectangleFPS);
        Serial.printf("Lines:           %.2f lines/sec\n", _results.lineFPS);
        Serial.printf("Circles:         %.2f circles/sec\n", _results.circleFPS);
        Serial.printf("Text:            %.0f chars/sec\n", _results.textCharPerSec);
        Serial.printf("Partial Updates: %.2f FPS\n", _results.partialUpdateFPS);
        Serial.println("[Benchmark] ========================");
    }
};

} // namespace Doki

#endif // DOKI_APP_BENCHMARK_H
