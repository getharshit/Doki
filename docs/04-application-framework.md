# Application Framework

This document provides a comprehensive guide to the Doki OS application framework, including app lifecycle, development guidelines, and best practices.

## Overview

The Doki OS application framework provides a structured approach to building interactive displays. All apps inherit from the `DokiApp` base class and follow a defined lifecycle.

## DokiApp Base Class

**Location**: [include/doki/app_base.h](../include/doki/app_base.h)

### Class Declaration

```cpp
namespace Doki {

class DokiApp {
public:
    DokiApp(const char* id, const char* name);
    virtual ~DokiApp();

    // Lifecycle methods (must override)
    virtual void onCreate() = 0;
    virtual void onStart() = 0;
    virtual void onUpdate() = 0;
    virtual void onDestroy() = 0;
    virtual void onPause() {};  // Optional

    // Getters
    const char* getId() const;
    const char* getName() const;
    AppState getState() const;
    uint32_t getUptime() const;
    bool isRunning() const;

protected:
    void clearScreen();
    lv_obj_t* getScreen();
    void log(const char* message);
};

}  // namespace Doki
```

## App Lifecycle

### Lifecycle States

```cpp
enum class AppState {
    IDLE,        // App not loaded
    CREATED,     // onCreate() called
    STARTED,     // onStart() called, app is visible
    PAUSED,      // onPause() called, app in background
    DESTROYED    // onDestroy() called
};
```

### State Transition Diagram

```
┌──────┐
│ IDLE │ (App not loaded)
└──┬───┘
   │ new MyApp()
   ↓
┌─────────┐
│ CREATED │ onCreate() - Create UI, initialize
└────┬────┘
     │
     ↓
┌─────────┐
│ STARTED │ onStart() - Start timers, animations
└────┬────┘
     │
     ↓
┌─────────┐  onUpdate() called every frame
│ RUNNING │◄─────────────────┐
└────┬────┘                  │
     │                       │
     │ (switching apps)      │
     ↓                       │
┌────────┐                   │
│ PAUSED │ onPause() - Stop timers (optional)
└────┬───┘
     │
     ↓
┌───────────┐
│ DESTROYED │ onDestroy() - Cleanup
└───────────┘
     │
     ↓
  (deleted)
```

### Lifecycle Methods

#### 1. onCreate()

**Purpose**: Initialize the app and create UI elements

**Called When**: App is first loaded onto a display

**Typical Tasks**:
- Create LVGL objects (labels, buttons, images, etc.)
- Load configuration or saved state
- Initialize variables
- Allocate memory if needed

**Example**:
```cpp
void onCreate() override {
    log("Creating Clock App");

    // Create time label
    _timeLabel = lv_label_create(getScreen());
    lv_label_set_text(_timeLabel, "00:00:00");
    lv_obj_center(_timeLabel);
    lv_obj_set_style_text_font(_timeLabel, &lv_font_montserrat_48, 0);

    _lastUpdate = 0;
}
```

**Important Notes**:
- LVGL context is already set for the correct display
- Screen is cleared before onCreate() is called
- Don't call blocking functions (delays, long loops)

#### 2. onStart()

**Purpose**: Start app execution (animations, timers, data fetching)

**Called When**: Immediately after onCreate()

**Typical Tasks**:
- Start FreeRTOS tasks
- Begin animations
- Subscribe to events
- Fetch initial data

**Example**:
```cpp
void onStart() override {
    log("Clock App started");

    // Start NTP sync in background
    _ntpClient->begin();
    xTaskCreate(ntpUpdateTask, "NTP", 4096, this, 1, &_ntpTaskHandle);
}
```

**Important Notes**:
- App is now visible on display
- User can see the UI created in onCreate()

#### 3. onUpdate()

**Purpose**: Update app state and UI

**Called When**: Every frame (~30-60 FPS in main loop)

**Typical Tasks**:
- Check elapsed time
- Update labels, progress bars, etc.
- Fetch data if needed
- Handle animations (though LVGL handles most animations)

**Example**:
```cpp
void onUpdate() override {
    uint32_t now = millis();

    // Update every 1 second
    if (now - _lastUpdate >= 1000) {
        updateTimeDisplay();
        _lastUpdate = now;
    }
}
```

**Important Notes**:
- Keep this method fast (<10ms execution time)
- Use time-based checks (don't update every frame)
- Don't use blocking operations (no delay()!)

#### 4. onPause() (Optional)

**Purpose**: Pause app when switching to another app

**Called When**: Another app is about to be loaded

**Typical Tasks**:
- Pause animations
- Stop timers
- Save state

**Example**:
```cpp
void onPause() override {
    log("Clock App paused");
    // In current implementation, apps are destroyed immediately
    // This method is reserved for future multi-app scenarios
}
```

**Important Notes**:
- Currently not heavily used (apps are unloaded immediately)
- Future enhancement for app backgrounding

#### 5. onDestroy()

**Purpose**: Clean up resources before app is unloaded

**Called When**: App is being replaced by another app

**Typical Tasks**:
- Stop FreeRTOS tasks
- Free allocated memory
- Close network connections
- Save persistent data

**Example**:
```cpp
void onDestroy() override {
    log("Clock App destroyed");

    // Stop background task
    if (_ntpTaskHandle) {
        vTaskDelete(_ntpTaskHandle);
        _ntpTaskHandle = nullptr;
    }

    // Free resources
    delete _ntpClient;
    delete _udp;

    // LVGL automatically cleans up UI elements
}
```

**Important Notes**:
- **LVGL objects are auto-cleaned**: Don't manually delete LVGL objects
- Free any memory you allocated
- Stop all background tasks

## Creating a New App

### Step 1: Create App Header File

**Location**: `src/apps/myapp/myapp.h`

**Template**:
```cpp
#ifndef MYAPP_H
#define MYAPP_H

#include "doki/app_base.h"

class MyApp : public Doki::DokiApp {
public:
    MyApp() : DokiApp("myapp", "My App") {
        // Constructor: Initialize member variables
    }

    void onCreate() override {
        // Create UI
    }

    void onStart() override {
        // Start app
    }

    void onUpdate() override {
        // Update app state
    }

    void onPause() override {
        // Pause (optional)
    }

    void onDestroy() override {
        // Cleanup
    }

private:
    // Member variables
    lv_obj_t* _myLabel;
    uint32_t _lastUpdate;
};

#endif // MYAPP_H
```

### Step 2: Register App in main.cpp

**Location**: [src/main.cpp](../src/main.cpp)

1. Include the header:
```cpp
#include "apps/myapp/myapp.h"
```

2. Add to `createApp()` function:
```cpp
Doki::DokiApp* createApp(const String& appId) {
    if (appId == "clock") return new ClockApp();
    if (appId == "weather") return new WeatherApp();
    if (appId == "myapp") return new MyApp();  // Add this line
    // ...
    return nullptr;
}
```

### Step 3: Add to Web Dashboard

**Location**: [src/main.cpp](../src/main.cpp) (inside dashboard HTML)

Add to JavaScript apps array:
```javascript
const apps = [
    { id: 'clock', name: '⏰ Clock' },
    { id: 'weather', name: '🌤️ Weather' },
    { id: 'myapp', name: '🎨 My App' },  // Add this line
    // ...
];
```

### Step 4: Build and Test

```bash
pio run --target upload
```

Open dashboard and click "My App" button to load it.

## Example Apps

### Minimal App

```cpp
class MinimalApp : public Doki::DokiApp {
public:
    MinimalApp() : DokiApp("minimal", "Minimal") {}

    void onCreate() override {
        lv_obj_t* label = lv_label_create(getScreen());
        lv_label_set_text(label, "Hello, Doki!");
        lv_obj_center(label);
    }

    void onStart() override {}
    void onUpdate() override {}
    void onDestroy() override {}
};
```

### Counter App (with update loop)

```cpp
class CounterApp : public Doki::DokiApp {
public:
    CounterApp() : DokiApp("counter", "Counter") {}

    void onCreate() override {
        _counterLabel = lv_label_create(getScreen());
        lv_label_set_text(_counterLabel, "0");
        lv_obj_center(_counterLabel);
        lv_obj_set_style_text_font(_counterLabel, &lv_font_montserrat_48, 0);

        _counter = 0;
        _lastUpdate = 0;
    }

    void onStart() override {
        log("Counter started");
    }

    void onUpdate() override {
        uint32_t now = millis();

        // Increment every second
        if (now - _lastUpdate >= 1000) {
            _counter++;
            char buf[16];
            snprintf(buf, sizeof(buf), "%d", _counter);
            lv_label_set_text(_counterLabel, buf);
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
};
```

### Image App

```cpp
#include "assets/myimage.h"  // Generated image header

class ImageApp : public Doki::DokiApp {
public:
    ImageApp() : DokiApp("image", "Image") {}

    void onCreate() override {
        lv_obj_t* img = lv_img_create(getScreen());
        lv_img_set_src(img, &myimage);
        lv_obj_center(img);
    }

    void onStart() override {}
    void onUpdate() override {}
    void onDestroy() override {}
};
```

## Best Practices

### 1. Non-Blocking Updates

**❌ Bad:**
```cpp
void onUpdate() override {
    delay(1000);  // BLOCKS entire system!
    updateDisplay();
}
```

**✅ Good:**
```cpp
void onUpdate() override {
    uint32_t now = millis();
    if (now - _lastUpdate >= 1000) {
        updateDisplay();
        _lastUpdate = now;
    }
}
```

### 2. Resource Management

**❌ Bad:**
```cpp
void onDestroy() override {
    // Forgot to stop task!
}
```

**✅ Good:**
```cpp
void onDestroy() override {
    if (_taskHandle) {
        vTaskDelete(_taskHandle);
        _taskHandle = nullptr;
    }
    delete _allocatedData;
}
```

### 3. Use Helper Methods

**❌ Bad:**
```cpp
void onCreate() override {
    Serial.printf("[MyApp] Creating UI\n");
}
```

**✅ Good:**
```cpp
void onCreate() override {
    log("Creating UI");  // Uses app-specific log format
}
```

### 4. Don't Manually Delete LVGL Objects

**❌ Bad:**
```cpp
void onDestroy() override {
    lv_obj_del(_myLabel);  // LVGL auto-cleans!
}
```

**✅ Good:**
```cpp
void onDestroy() override {
    // LVGL objects are automatically cleaned up
    // Only cleanup non-LVGL resources
    delete _myData;
}
```

### 5. Initialize All Variables

**❌ Bad:**
```cpp
class MyApp : public Doki::DokiApp {
private:
    lv_obj_t* _label;  // Uninitialized!
    uint32_t _lastUpdate;  // Uninitialized!
};
```

**✅ Good:**
```cpp
class MyApp : public Doki::DokiApp {
public:
    MyApp() : DokiApp("myapp", "My App") {
        _label = nullptr;
        _lastUpdate = 0;
    }
};
```

## Advanced Techniques

### 1. Background Tasks

For long-running operations (network, calculations):

```cpp
class DataApp : public Doki::DokiApp {
    void onStart() override {
        xTaskCreate(dataFetchTask, "DataFetch", 4096, this, 1, &_taskHandle);
    }

    static void dataFetchTask(void* param) {
        DataApp* self = (DataApp*)param;

        while (true) {
            // Fetch data (this can block)
            self->fetchData();

            vTaskDelay(pdMS_TO_TICKS(60000));  // Wait 1 minute
        }
    }

    void onDestroy() override {
        if (_taskHandle) vTaskDelete(_taskHandle);
    }

private:
    TaskHandle_t _taskHandle = nullptr;
};
```

### 2. Using Services

```cpp
#include "doki/weather_service.h"

void onCreate() override {
    // Service is already initialized in main.cpp
}

void fetchWeather() {
    Doki::WeatherData data;
    if (Doki::WeatherService::getCurrentWeather("Mumbai", data)) {
        // Update UI with weather data
        char temp[32];
        snprintf(temp, sizeof(temp), "%.1f°C", data.tempC);
        lv_label_set_text(_tempLabel, temp);
    }
}
```

### 3. Animations

```cpp
void onCreate() override {
    _label = lv_label_create(getScreen());
    lv_label_set_text(_label, "Animated!");

    // Create fade-in animation
    lv_anim_t anim;
    lv_anim_init(&anim);
    lv_anim_set_var(&anim, _label);
    lv_anim_set_values(&anim, 0, 255);  // Opacity 0 → 255
    lv_anim_set_time(&anim, 1000);      // 1 second
    lv_anim_set_exec_cb(&anim, [](void* obj, int32_t v) {
        lv_obj_set_style_opa((lv_obj_t*)obj, v, 0);
    });
    lv_anim_start(&anim);
}
```

## Debugging Apps

### Enable Logging

```cpp
void onCreate() override {
    log("onCreate called");
}

void onUpdate() override {
    log("onUpdate called");  // WARNING: Called 30-60 times per second!
}
```

### Monitor Memory

Use System Info app to check heap and PSRAM usage.

### Serial Output

All `log()` calls output to serial:

```
[DokiApp:myapp] onCreate called
[DokiApp:myapp] onStart called
```

## Common Pitfalls

### 1. Forgetting to Call Base Constructor

**❌ Bad:**
```cpp
class MyApp : public Doki::DokiApp {
public:
    MyApp() {  // Missing base constructor!
        // ...
    }
};
```

**✅ Good:**
```cpp
class MyApp : public Doki::DokiApp {
public:
    MyApp() : DokiApp("myapp", "My App") {
        // ...
    }
};
```

### 2. Heavy Operations in onUpdate()

**❌ Bad:**
```cpp
void onUpdate() override {
    // Called 60 times per second!
    fetchWeatherFromInternet();  // Too slow!
}
```

**✅ Good:**
```cpp
void onUpdate() override {
    uint32_t now = millis();
    if (now - _lastFetch >= 600000) {  // Every 10 minutes
        fetchWeatherFromInternet();
        _lastFetch = now;
    }
}
```

### 3. Using delay()

**❌ Bad:**
```cpp
void onUpdate() override {
    delay(1000);  // FREEZES ENTIRE SYSTEM!
}
```

**✅ Good:**
```cpp
// Use time-based checks instead
if (millis() - _lastUpdate >= 1000) {
    // Do work
}
```

## Next Steps

- [Display Management](./05-display-management.md) - LVGL and display handling
- [Services](./06-services.md) - Using weather and time services
- [App Development Guide](./08-app-development.md) - Advanced app development

---

[← Back: System Architecture](./03-system-architecture.md) | [Next: Display Management →](./05-display-management.md)
