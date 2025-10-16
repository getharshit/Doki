# Technical Notes

This document contains detailed technical notes about critical bugs, fixes, and design decisions in Doki OS.

## Critical Bug Fixes

### 1. Random Crash When Switching Apps (Fixed: 2025-10-16)

#### Symptom
```
Guru Meditation Error: Core 1 panic'ed (LoadProhibited). Exception was unhandled.
A2: 0x00000000  (null pointer)
EXCVADDR: 0x00000008  (attempting to access offset +8 from null pointer)
```

The system would randomly crash when switching between apps, typically after successfully loading 2-3 apps in sequence.

#### Root Cause

The bug was a **race condition between LVGL's task handler and app cleanup**:

1. Original sequence in `loadAppOnDisplay()`:
   ```cpp
   // BROKEN sequence:
   if (d->app) {
       d->app->onDestroy();  // App still has UI object pointers
       delete d->app;
   }
   // ... create new app ...
   lv_obj_clean(screen);     // ← Destroys all LVGL objects
   d->app->onCreate();       // ← Creates new objects
   ```

2. **The Problem**: Between `lv_obj_clean()` and `onCreate()`, if LVGL's task handler (`lv_timer_handler()`) ran, it could:
   - Trigger `onUpdate()` callbacks on the **old app instance**
   - The old app's member variables (`_label`, `_uptimeLabel`, etc.) still pointed to **deleted LVGL objects**
   - Accessing these deleted objects caused `LoadProhibited` crash at null pointer

3. **Example crash scenario** (HelloApp):
   ```cpp
   // HelloApp stores these pointers:
   lv_obj_t* _label;
   lv_obj_t* _uptimeLabel;

   // In onUpdate() - called by LVGL task handler:
   void onUpdate() {
       lv_label_set_text(_uptimeLabel, buf);  // ← CRASH: _uptimeLabel points to deleted memory
   }
   ```

#### The Fix

**Reorder the cleanup sequence** to clean the screen **before** destroying the app:

```cpp
// FIXED sequence in loadAppOnDisplay():
lv_disp_set_default(d->disp);

// 1. Clean screen FIRST - removes all LVGL objects
lv_obj_t* screen = lv_disp_get_scr_act(d->disp);
lv_obj_clean(screen);

// 2. NOW destroy the old app - its UI objects are already gone
if (d->app) {
    d->app->onDestroy();
    delete d->app;
    d->app = nullptr;
}

// 3. Create and initialize new app on clean screen
d->app = createApp(appId);
d->app->onCreate();
d->app->onStart();
```

**Why this works**:
- When `lv_obj_clean()` runs first, all LVGL objects are destroyed immediately
- The old app's pointers now point to non-existent objects, **but**:
- We immediately destroy the old app instance **before** LVGL's task handler runs again
- No callbacks can be triggered on deleted objects
- The new app starts with a clean slate

#### Lessons Learned

1. **LVGL is event-driven**: The task handler can trigger callbacks at any time
2. **Order matters**: Clean up UI objects before destroying the app instance
3. **Race conditions are subtle**: The crash only happened intermittently because it depended on timing between LVGL's task handler and app switching
4. **Dangling pointers**: Always be aware of what pointers your app stores and when they become invalid

#### Related Code

- Fixed in: [src/main.cpp:395-416](../src/main.cpp#L395-L416)
- Affects: All apps that store LVGL object pointers (HelloApp, GoodbyeApp, ClockApp, etc.)
- Architecture: [docs/03-system-architecture.md - App Loading Flow](./03-system-architecture.md#app-loading-flow)

---

## Design Decisions

### 1. HTTP Server Architecture: Callback Pattern

**Decision**: Use callback-based architecture in SimpleHttpServer instead of direct dependencies.

**Rationale**:
- Decouples HTTP server from main application logic
- HTTP server doesn't need to know about Display structures or app management
- Makes the server module reusable and testable
- Follows dependency inversion principle

**Implementation**:
```cpp
// In SimpleHttpServer:
static bool (*_loadAppCallback)(uint8_t, const String&);
static void (*_statusCallback)(uint8_t, String&, uint32_t&);

// In main.cpp:
SimpleHttpServer::setLoadAppCallback(loadAppOnDisplay);
SimpleHttpServer::setStatusCallback(getDisplayStatus);
```

**Alternative Considered**: Direct include of main.cpp functions
- Rejected: Creates tight coupling, circular dependencies, harder to test

---

### 2. JavaScript: No Template Literals in C++ Rawliteral Strings

**Decision**: Use string concatenation instead of ES6 template literals in embedded JavaScript.

**Rationale**:
- Template literals use `${}` syntax which confuses C++ compiler inside `R"rawliteral()"` strings
- Causes multiple compilation errors: "identifier $ is undefined", "unrecognized token"
- String concatenation is ES5-compatible and works reliably in C++ embedded strings

**Implementation**:
```javascript
// ❌ AVOID: Template literals
const url = `/api/load?display=${displayId}&app=${appId}`;

// ✅ USE: String concatenation
const url = '/api/load?display=' + displayId + '&app=' + appId;
```

**Trade-off**: Slightly less readable, but guaranteed compatibility

---

### 3. Open WiFi Support: Space Character Workaround

**Decision**: Save empty WiFi passwords as a space character `" "` in NVS storage.

**Rationale**:
- ESP32 NVS `putString()` fails when given empty string
- Open WiFi networks (no password) are common
- Space character is a valid workaround that doesn't break WiFi connection

**Implementation**:
```cpp
// Save
String passwordToSave = password.isEmpty() ? " " : password;
_prefs.putString(KEY_WIFI_PASSWORD, passwordToSave);

// Load
String password = _prefs.getString(KEY_WIFI_PASSWORD, "");
if (password == " ") password = "";
```

**Alternative Considered**: Use a flag to indicate "no password"
- Rejected: Adds complexity, space character is simpler and works

---

### 4. Display Initialization: Hybrid Approach

**Decision**: Keep original working display initialization code instead of fully abstracting into DisplayManager.

**Rationale**:
- Original code in main.cpp was proven to work correctly
- Early refactoring attempts caused display crashes ("no display registered")
- ST7789 initialization is hardware-specific and timing-sensitive
- Benefits of abstraction didn't outweigh risk of bugs

**Current State**: Display init code remains in main.cpp with LVGL integration

**Future**: Consider moving to DisplayManager once proven stable in different configurations

---

## Performance Optimizations

### 1. Weather API Caching (10 minutes)

**Implementation**: Cache weather data for 10 minutes to reduce API calls
- Saves bandwidth and API quota
- Weather doesn't change frequently enough to warrant constant updates
- User can manually refresh if needed

**Code**: [src/doki/weather_service.cpp](../src/doki/weather_service.cpp)

---

### 2. Watchdog Timer Management

**Problem**: Long operations (WiFi scan, HTTP requests) trigger watchdog timeout

**Solution**: Feed watchdog during long operations:
```cpp
esp_task_wdt_reset();  // Feed the watchdog
WiFi.scanNetworks(false, true, false, 120);  // Reduced scan time to 120ms
esp_task_wdt_reset();  // Feed again after operation
```

**Note**: WiFi scanning was later removed per user request, but the pattern remains important for other long operations.

---

## Known Limitations

### 1. Single-Threaded LVGL

**Limitation**: LVGL is not thread-safe by default

**Impact**: All LVGL operations must be protected with mutex when accessed from multiple tasks

**Current State**: Apps run in main loop (single-threaded), so no mutex needed yet

**Future**: If adding background tasks that modify UI, implement LVGL mutex:
```cpp
SemaphoreHandle_t lvgl_mutex;
xSemaphoreTake(lvgl_mutex, portMAX_DELAY);
// LVGL operations here
xSemaphoreGive(lvgl_mutex);
```

---

### 2. PSRAM Buffer Allocation

**Implementation**: Display buffers allocated in PSRAM using `heap_caps_malloc()`

**Limitation**: PSRAM is slower than internal RAM
- May cause slight performance degradation for fast animations
- Trade-off for supporting dual displays with large buffers (~76KB total)

**Alternative**: Use smaller buffers in internal RAM
- Rejected: Would require more complex buffer management and partial rendering

---

## Security Considerations

### 1. Hardcoded Weather API Key

**Current State**: API key hardcoded in main.cpp:
```cpp
#define WEATHER_API_KEY "3183db8ec2fe4abfa2c133226251310"
```

**Risk**: Key exposed in source code and firmware

**Recommendation**:
- Store in NVS (user-configurable via dashboard)
- Use environment variables during build
- Implement key rotation

**Priority**: Low (this is a personal project, free tier API)

---

### 2. Open WiFi Access Point

**Current State**: AP mode with password, but HTTP server has no authentication

**Risk**: Anyone on the WiFi network can control the displays

**Recommendation**:
- Add HTTP basic auth
- Implement session tokens
- Rate limiting on API endpoints

**Priority**: Medium (depends on deployment environment)

---

## Future Improvements

### 1. App Registry/Discovery

**Current State**: Apps are manually registered in `createApp()` function

**Proposal**: Implement automatic app discovery:
```cpp
// Apps self-register on initialization
REGISTER_APP(ClockApp, "clock", "Clock");
REGISTER_APP(WeatherApp, "weather", "Weather");
```

**Benefits**: Easier to add new apps, no need to modify central registry

---

### 2. Persistent App State

**Current State**: App state is lost when switching apps

**Proposal**: Add save/restore lifecycle methods:
```cpp
virtual void onSaveState(JsonObject& state) {};
virtual void onRestoreState(JsonObject& state) {};
```

**Use Case**: Remember scroll position, settings, user data when returning to an app

---

### 3. Inter-App Communication

**Current State**: Apps are completely isolated

**Proposal**: Message bus for app-to-app communication:
```cpp
MessageBus::subscribe("time_update", myCallback);
MessageBus::publish("time_update", data);
```

**Use Case**: Clock app broadcasts time, other apps can subscribe

---

## Debugging Tips

### 1. Reading Stack Traces

When you get a Guru Meditation Error:
```
Backtrace: 0x420ce0ec:0x3fcebe20 0x4200bd4c:0x3fcebe40
```

Use ESP32 exception decoder or `xtensa-esp32s3-elf-addr2line`:
```bash
xtensa-esp32s3-elf-addr2line -e .pio/build/esp32-s3-devkitc-1/firmware.elf 0x420ce0ec
```

This will show you the exact source file and line number.

---

### 2. Memory Leak Detection

Monitor heap usage in System Info app or serial output:
```cpp
Serial.printf("Free heap: %d bytes\n", ESP.getFreeHeap());
Serial.printf("Free PSRAM: %d bytes\n", ESP.getFreePsram());
```

If memory continuously decreases, check for:
- Missing `delete` in `onDestroy()`
- LVGL objects not cleaned up
- Unclosed HTTP connections

---

### 3. LVGL Debugging

Enable LVGL logging in `lv_conf.h`:
```c
#define LV_USE_LOG 1
#define LV_LOG_LEVEL LV_LOG_LEVEL_WARN
```

This will show LVGL warnings about:
- Invalid object access
- Drawing errors
- Memory issues

---

## Version History

- **v0.2.0** (2025-10-16): Fixed random crash bug, added SimpleHttpServer module
- **v0.1.0** (Initial): Basic dual display system with modular architecture

---

[← Back to Index](./INDEX.md)
