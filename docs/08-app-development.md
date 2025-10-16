# App Development Guide

This guide provides practical examples and step-by-step instructions for developing apps for Doki OS.

## Quick Start: Your First App

Let's create a simple "Hello World" app from scratch.

### Step 1: Create the App File

Create `src/apps/helloworld/helloworld.h`:

```cpp
#ifndef HELLOWORLD_H
#define HELLOWORLD_H

#include "doki/app_base.h"

class HelloWorldApp : public Doki::DokiApp {
public:
    HelloWorldApp() : DokiApp("helloworld", "Hello World") {}

    void onCreate() override {
        // Create a label
        lv_obj_t* label = lv_label_create(getScreen());
        lv_label_set_text(label, "Hello, Doki OS!");
        lv_obj_center(label);
        lv_obj_set_style_text_font(label, &lv_font_montserrat_24, 0);
        lv_obj_set_style_text_color(label, lv_color_hex(0x667eea), 0);
    }

    void onStart() override {
        log("Hello World started");
    }

    void onUpdate() override {
        // Nothing to update
    }

    void onDestroy() override {
        log("Hello World destroyed");
    }
};

#endif
```

### Step 2: Register in main.cpp

Edit [src/main.cpp](../src/main.cpp):

```cpp
// 1. Include at top
#include "apps/helloworld/helloworld.h"

// 2. Add to createApp() function
Doki::DokiApp* createApp(const String& appId) {
    if (appId == "clock") return new ClockApp();
    if (appId == "weather") return new WeatherApp();
    if (appId == "helloworld") return new HelloWorldApp();  // ← Add this
    // ...
}
```

### Step 3: Add to Dashboard

In [src/main.cpp](../src/main.cpp), find the dashboard HTML and add to the apps array:

```javascript
const apps = [
    { id: 'clock', name: '⏰ Clock' },
    { id: 'weather', name: '🌤️ Weather' },
    { id: 'helloworld', name: '👋 Hello' },  // ← Add this
    // ...
];
```

### Step 4: Build and Test

```bash
pio run --target upload
pio device monitor
```

Open the dashboard and click "👋 Hello" to see your app!

## Practical Examples

### Example 1: Counter with Button

An app that increments a counter when you (virtually) press a button.

```cpp
#ifndef COUNTER_APP_H
#define COUNTER_APP_H

#include "doki/app_base.h"

class CounterApp : public Doki::DokiApp {
public:
    CounterApp() : DokiApp("counter", "Counter") {
        _counter = 0;
        _lastUpdate = 0;
    }

    void onCreate() override {
        // Title
        lv_obj_t* title = lv_label_create(getScreen());
        lv_label_set_text(title, "Counter App");
        lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 20);
        lv_obj_set_style_text_font(title, &lv_font_montserrat_18, 0);

        // Counter display
        _counterLabel = lv_label_create(getScreen());
        lv_label_set_text(_counterLabel, "0");
        lv_obj_center(_counterLabel);
        lv_obj_set_style_text_font(_counterLabel, &lv_font_montserrat_48, 0);
        lv_obj_set_style_text_color(_counterLabel, lv_color_hex(0x10b981), 0);

        // Info
        lv_obj_t* info = lv_label_create(getScreen());
        lv_label_set_text(info, "Auto-increments every second");
        lv_obj_align(info, LV_ALIGN_BOTTOM_MID, 0, -20);
        lv_obj_set_style_text_font(info, &lv_font_montserrat_10, 0);
    }

    void onStart() override {
        log("Counter started");
    }

    void onUpdate() override {
        uint32_t now = millis();

        // Increment every second
        if (now - _lastUpdate >= 1000) {
            _counter++;
            updateDisplay();
            _lastUpdate = now;
        }
    }

    void onDestroy() override {
        log("Counter destroyed");
    }

private:
    lv_obj_t* _counterLabel;
    int _counter;
    uint32_t _lastUpdate;

    void updateDisplay() {
        char buf[32];
        snprintf(buf, sizeof(buf), "%d", _counter);
        lv_label_set_text(_counterLabel, buf);
    }
};

#endif
```

### Example 2: Progress Bar Animation

An app showing animated progress.

```cpp
#ifndef PROGRESS_APP_H
#define PROGRESS_APP_H

#include "doki/app_base.h"

class ProgressApp : public Doki::DokiApp {
public:
    ProgressApp() : DokiApp("progress", "Progress") {
        _progress = 0;
        _lastUpdate = 0;
    }

    void onCreate() override {
        // Title
        lv_obj_t* title = lv_label_create(getScreen());
        lv_label_set_text(title, "Progress Demo");
        lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 20);

        // Progress bar
        _progressBar = lv_bar_create(getScreen());
        lv_obj_set_size(_progressBar, 200, 30);
        lv_obj_center(_progressBar);
        lv_bar_set_range(_progressBar, 0, 100);
        lv_bar_set_value(_progressBar, 0, LV_ANIM_OFF);
        lv_obj_set_style_bg_color(_progressBar, lv_color_hex(0xe5e7eb), 0);
        lv_obj_set_style_bg_color(_progressBar, lv_color_hex(0x667eea), LV_PART_INDICATOR);

        // Percentage label
        _percentLabel = lv_label_create(getScreen());
        lv_label_set_text(_percentLabel, "0%");
        lv_obj_align(_percentLabel, LV_ALIGN_CENTER, 0, 40);
        lv_obj_set_style_text_font(_percentLabel, &lv_font_montserrat_20, 0);
    }

    void onStart() override {
        log("Progress started");
    }

    void onUpdate() override {
        uint32_t now = millis();

        // Update every 100ms
        if (now - _lastUpdate >= 100) {
            _progress += 1;
            if (_progress > 100) _progress = 0;  // Loop

            updateDisplay();
            _lastUpdate = now;
        }
    }

    void onDestroy() override {
        log("Progress destroyed");
    }

private:
    lv_obj_t* _progressBar;
    lv_obj_t* _percentLabel;
    int _progress;
    uint32_t _lastUpdate;

    void updateDisplay() {
        lv_bar_set_value(_progressBar, _progress, LV_ANIM_OFF);

        char buf[16];
        snprintf(buf, sizeof(buf), "%d%%", _progress);
        lv_label_set_text(_percentLabel, buf);
    }
};

#endif
```

### Example 3: Multi-Screen App with Data

An app that displays system information.

```cpp
#ifndef INFO_APP_H
#define INFO_APP_H

#include "doki/app_base.h"
#include <WiFi.h>

class InfoApp : public Doki::DokiApp {
public:
    InfoApp() : DokiApp("info", "Info") {
        _lastUpdate = 0;
    }

    void onCreate() override {
        // Title
        _titleLabel = lv_label_create(getScreen());
        lv_label_set_text(_titleLabel, "System Information");
        lv_obj_align(_titleLabel, LV_ALIGN_TOP_MID, 0, 10);
        lv_obj_set_style_text_font(_titleLabel, &lv_font_montserrat_16, 0);

        // Heap memory
        _heapLabel = lv_label_create(getScreen());
        lv_label_set_text(_heapLabel, "Heap: --- KB");
        lv_obj_align(_heapLabel, LV_ALIGN_TOP_LEFT, 20, 50);

        // PSRAM
        _psramLabel = lv_label_create(getScreen());
        lv_label_set_text(_psramLabel, "PSRAM: --- KB");
        lv_obj_align(_psramLabel, LV_ALIGN_TOP_LEFT, 20, 80);

        // WiFi
        _wifiLabel = lv_label_create(getScreen());
        lv_label_set_text(_wifiLabel, "WiFi: ---");
        lv_obj_align(_wifiLabel, LV_ALIGN_TOP_LEFT, 20, 110);

        // IP Address
        _ipLabel = lv_label_create(getScreen());
        lv_label_set_text(_ipLabel, "IP: ---");
        lv_obj_align(_ipLabel, LV_ALIGN_TOP_LEFT, 20, 140);

        // Uptime
        _uptimeLabel = lv_label_create(getScreen());
        lv_label_set_text(_uptimeLabel, "Uptime: ---");
        lv_obj_align(_uptimeLabel, LV_ALIGN_TOP_LEFT, 20, 170);
    }

    void onStart() override {
        log("Info app started");
        updateInfo();  // Initial update
    }

    void onUpdate() override {
        uint32_t now = millis();

        // Update every 2 seconds
        if (now - _lastUpdate >= 2000) {
            updateInfo();
            _lastUpdate = now;
        }
    }

    void onDestroy() override {
        log("Info app destroyed");
    }

private:
    lv_obj_t* _titleLabel;
    lv_obj_t* _heapLabel;
    lv_obj_t* _psramLabel;
    lv_obj_t* _wifiLabel;
    lv_obj_t* _ipLabel;
    lv_obj_t* _uptimeLabel;
    uint32_t _lastUpdate;

    void updateInfo() {
        char buf[64];

        // Heap memory
        uint32_t freeHeap = ESP.getFreeHeap() / 1024;
        uint32_t totalHeap = ESP.getHeapSize() / 1024;
        snprintf(buf, sizeof(buf), "Heap: %lu / %lu KB", freeHeap, totalHeap);
        lv_label_set_text(_heapLabel, buf);

        // PSRAM
        uint32_t freePsram = ESP.getFreePsram() / 1024;
        uint32_t totalPsram = ESP.getPsramSize() / 1024;
        snprintf(buf, sizeof(buf), "PSRAM: %lu / %lu KB", freePsram, totalPsram);
        lv_label_set_text(_psramLabel, buf);

        // WiFi status
        if (WiFi.status() == WL_CONNECTED) {
            int rssi = WiFi.RSSI();
            snprintf(buf, sizeof(buf), "WiFi: Connected (%d dBm)", rssi);
        } else {
            snprintf(buf, sizeof(buf), "WiFi: Disconnected");
        }
        lv_label_set_text(_wifiLabel, buf);

        // IP address
        if (WiFi.status() == WL_CONNECTED) {
            snprintf(buf, sizeof(buf), "IP: %s", WiFi.localIP().toString().c_str());
        } else {
            snprintf(buf, sizeof(buf), "IP: ---");
        }
        lv_label_set_text(_ipLabel, buf);

        // Uptime
        uint32_t uptime = millis() / 1000;
        uint32_t hours = uptime / 3600;
        uint32_t minutes = (uptime % 3600) / 60;
        uint32_t seconds = uptime % 60;
        snprintf(buf, sizeof(buf), "Uptime: %luh %lum %lus", hours, minutes, seconds);
        lv_label_set_text(_uptimeLabel, buf);
    }
};

#endif
```

## LVGL UI Components

### Common Widgets

#### Labels

```cpp
lv_obj_t* label = lv_label_create(getScreen());
lv_label_set_text(label, "Hello");
lv_obj_align(label, LV_ALIGN_CENTER, 0, 0);  // Center
lv_obj_set_style_text_font(label, &lv_font_montserrat_24, 0);
lv_obj_set_style_text_color(label, lv_color_hex(0xFF0000), 0);  // Red
```

#### Progress Bars

```cpp
lv_obj_t* bar = lv_bar_create(getScreen());
lv_obj_set_size(bar, 200, 20);
lv_bar_set_range(bar, 0, 100);
lv_bar_set_value(bar, 50, LV_ANIM_OFF);  // Set to 50%
lv_obj_set_style_bg_color(bar, lv_color_hex(0x667eea), LV_PART_INDICATOR);
```

#### Images

```cpp
#include "assets/myimage.h"

lv_obj_t* img = lv_img_create(getScreen());
lv_img_set_src(img, &myimage);
lv_obj_center(img);
```

#### Arcs (Circular Progress)

```cpp
lv_obj_t* arc = lv_arc_create(getScreen());
lv_obj_set_size(arc, 150, 150);
lv_arc_set_range(arc, 0, 100);
lv_arc_set_value(arc, 75);
lv_obj_center(arc);
```

### Styling

```cpp
// Colors
lv_obj_set_style_bg_color(obj, lv_color_hex(0x667eea), 0);
lv_obj_set_style_text_color(obj, lv_color_hex(0xFFFFFF), 0);
lv_obj_set_style_border_color(obj, lv_color_hex(0x000000), 0);

// Opacity
lv_obj_set_style_bg_opa(obj, LV_OPA_50, 0);  // 50% transparent

// Borders
lv_obj_set_style_border_width(obj, 2, 0);
lv_obj_set_style_radius(obj, 10, 0);  // Rounded corners

// Padding
lv_obj_set_style_pad_all(obj, 10, 0);
```

### Alignment

```cpp
// Absolute position
lv_obj_set_pos(obj, 50, 100);  // x=50, y=100

// Relative alignment
lv_obj_align(obj, LV_ALIGN_CENTER, 0, 0);        // Center
lv_obj_align(obj, LV_ALIGN_TOP_MID, 0, 20);     // Top center, 20px down
lv_obj_align(obj, LV_ALIGN_BOTTOM_LEFT, 10, -10); // Bottom left corner
```

### Animations

```cpp
lv_anim_t anim;
lv_anim_init(&anim);
lv_anim_set_var(&anim, myObject);
lv_anim_set_values(&anim, 0, 255);  // Start and end values
lv_anim_set_time(&anim, 1000);      // Duration (ms)
lv_anim_set_exec_cb(&anim, [](void* obj, int32_t value) {
    lv_obj_set_style_opa((lv_obj_t*)obj, value, 0);
});
lv_anim_start(&anim);
```

## Working with Services

### Weather Service

```cpp
#include "doki/weather_service.h"

void fetchWeather() {
    Doki::WeatherData data;

    if (Doki::WeatherService::getCurrentWeather("Mumbai", data)) {
        // Success - update UI
        char temp[32];
        snprintf(temp, sizeof(temp), "%.1f°C", data.tempC);
        lv_label_set_text(_tempLabel, temp);

        lv_label_set_text(_condLabel, data.condition.c_str());

        char humidity[32];
        snprintf(humidity, sizeof(humidity), "Humidity: %d%%", data.humidity);
        lv_label_set_text(_humidityLabel, humidity);
    } else {
        // Failed
        lv_label_set_text(_tempLabel, "Error");
    }
}
```

### NTP Time (Background Task)

```cpp
#include <NTPClient.h>
#include <WiFiUdp.h>

class MyClockApp : public Doki::DokiApp {
private:
    NTPClient* _ntpClient;
    WiFiUDP* _udp;
    TaskHandle_t _ntpTaskHandle;

    static void ntpTask(void* param) {
        MyClockApp* self = (MyClockApp*)param;

        // Initial sync
        self->_ntpClient->forceUpdate();

        // Periodic updates
        while (true) {
            vTaskDelay(pdMS_TO_TICKS(60000));  // 1 minute
            self->_ntpClient->update();
        }
    }

    void onStart() override {
        _udp = new WiFiUDP();
        _ntpClient = new NTPClient(*_udp, "pool.ntp.org", 19800);  // UTC+5:30
        _ntpClient->begin();

        xTaskCreate(ntpTask, "NTP", 4096, this, 1, &_ntpTaskHandle);
    }

    void onDestroy() override {
        if (_ntpTaskHandle) vTaskDelete(_ntpTaskHandle);
        delete _ntpClient;
        delete _udp;
    }
};
```

## Performance Tips

### 1. Throttle Updates

```cpp
void onUpdate() override {
    uint32_t now = millis();

    // Only update every 100ms (10 FPS)
    if (now - _lastUpdate >= 100) {
        updateDisplay();
        _lastUpdate = now;
    }
}
```

### 2. Use LVGL Animations

LVGL handles animations efficiently:

```cpp
// ✅ Good: LVGL handles it
lv_bar_set_value(bar, 75, LV_ANIM_ON);

// ❌ Bad: Manual animation
for (int i = 0; i <= 75; i++) {
    lv_bar_set_value(bar, i, LV_ANIM_OFF);
    delay(10);  // Also, never use delay()!
}
```

### 3. Reduce Redraws

Only update when necessary:

```cpp
void updateTemp(float newTemp) {
    if (abs(newTemp - _lastTemp) > 0.5) {  // Only if changed significantly
        char buf[16];
        snprintf(buf, sizeof(buf), "%.1f°C", newTemp);
        lv_label_set_text(_tempLabel, buf);
        _lastTemp = newTemp;
    }
}
```

## Debugging

### Enable Debug Logging

```cpp
void onCreate() override {
    log("Creating app...");
}

void onUpdate() override {
    // WARNING: This logs 30-60 times per second!
    // log("Update called");

    // Better: Throttled logging
    if (millis() - _lastLog >= 5000) {
        log("Still running");
        _lastLog = millis();
    }
}
```

### Memory Monitoring

```cpp
void onCreate() override {
    Serial.printf("[Memory] Free heap: %u bytes\n", ESP.getFreeHeap());
    Serial.printf("[Memory] Free PSRAM: %u bytes\n", ESP.getFreePsram());
}
```

### LVGL Debug

Enable in [include/lv_conf.h](../include/lv_conf.h):

```c
#define LV_USE_LOG 1
#define LV_LOG_LEVEL LV_LOG_LEVEL_WARN
```

## Publishing Your App

### Checklist

- [ ] App header file in `src/apps/myapp/myapp.h`
- [ ] All lifecycle methods implemented
- [ ] No memory leaks (cleanup in `onDestroy()`)
- [ ] No blocking operations (no `delay()`)
- [ ] Registered in `createApp()` in main.cpp
- [ ] Added to dashboard app list
- [ ] Tested on hardware
- [ ] Performance is acceptable (<10ms per update)

### Sharing

1. Fork the Doki OS repository
2. Add your app in a new branch
3. Create a pull request
4. Include screenshot and description

## Next Steps

- Study existing apps: [src/apps/](../src/apps/)
- Read LVGL docs: https://docs.lvgl.io/8.3/
- Experiment with different UI layouts
- Share your creations!

---

[← Back: Application Framework](./04-application-framework.md) | [Main Documentation](./README.md)
