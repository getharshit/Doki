# TimeService - Developer Guide

## Overview

**TimeService** is a singleton service that provides centralized NTP time synchronization for the entire Doki OS system. It replaces the previous approach where multiple apps created their own NTP clients, which caused UDP conflicts and DNS errors.

## Why TimeService?

### Problems Solved

**Before TimeService**:
- Multiple NTP clients (js_engine.cpp, clock_app.h, etc.)
- UDP DNS errors every 60 seconds: `could not get host from dns: 11`
- Inconsistent time between apps
- Wasted RAM (~10KB per duplicate client)
- Race conditions in NTP sync

**With TimeService**:
- ✅ Single NTP client for entire system
- ✅ Zero UDP DNS errors
- ✅ Consistent time across all apps
- ✅ ~10KB RAM saved
- ✅ Thread-safe access
- ✅ Background synchronization

## Architecture

### Class Structure

```cpp
class TimeService {
public:
    // Singleton access
    static TimeService& getInstance();

    // Lifecycle
    bool begin();                    // Initialize and start sync task

    // Time access
    bool isTimeSynced() const;      // Check if time is synced
    struct tm getLocalTime();       // Get time as struct tm
    String getTimeString();         // Get formatted time (HH:MM:SS)
    String getDateString();         // Get formatted date (DD/MM/YYYY)

private:
    NTPClient* _ntpClient;          // Single NTP client
    WiFiUDP* _udp;                  // UDP connection
    TaskHandle_t _syncTaskHandle;   // Background task
    bool _initialized;              // Init flag
    volatile bool _synced;          // Sync status (thread-safe)

    static void syncTask(void*);    // Background sync task
};
```

### Background Synchronization

TimeService runs a background FreeRTOS task on **Core 0** (network core) that:

1. **Initial Sync** (on startup):
   - Waits 2 seconds for WiFi stability
   - Performs blocking sync (in background, doesn't freeze main app)
   - Sets `_synced` flag on success

2. **Periodic Updates**:
   - **When synced**: Updates every **1 hour** (3600000ms)
   - **When not synced**: Retries every **1 minute** (60000ms)
   - Non-blocking updates
   - Silent operation (no console spam)

3. **Error Handling**:
   - Suppresses UDP error logs to avoid console spam
   - Continues retrying on failure
   - Thread-safe flag updates

## Usage Guide

### 1. Initialization (main.cpp)

TimeService must be initialized once in `main.cpp` after WiFi is connected:

```cpp
void enterNormalMode() {
    // Connect to WiFi first
    WiFi.begin(STATION_SSID, STATION_PASSWORD);
    while (WiFi.status() != WL_CONNECTED) {
        delay(100);
    }

    // Initialize TimeService (starts background sync)
    Doki::TimeService::getInstance().begin();

    // Initialize other services...
}
```

### 2. Checking Sync Status

Always check if time is synced before using it:

```cpp
if (Doki::TimeService::getInstance().isTimeSynced()) {
    // Time is valid, safe to use
} else {
    // Time not yet synced, show "Syncing..." or similar
}
```

### 3. Getting Time

#### Option A: Structured Time (struct tm)

```cpp
struct tm timeInfo = Doki::TimeService::getInstance().getLocalTime();

if (timeInfo.tm_year > 0) {  // Validate
    int hour = timeInfo.tm_hour;        // 0-23
    int minute = timeInfo.tm_min;       // 0-59
    int second = timeInfo.tm_sec;       // 0-59
    int day = timeInfo.tm_mday;         // 1-31
    int month = timeInfo.tm_mon + 1;    // 1-12 (tm_mon is 0-11!)
    int year = timeInfo.tm_year + 1900; // Full year
    int weekday = timeInfo.tm_wday;     // 0-6 (0=Sunday)
}
```

#### Option B: Formatted Strings

```cpp
// Get time as "HH:MM:SS" (24-hour format)
String timeStr = Doki::TimeService::getInstance().getTimeString();
// Example: "14:35:22"

// Get date as "DD/MM/YYYY"
String dateStr = Doki::TimeService::getInstance().getDateString();
// Example: "17/10/2025"
```

### 4. Using in Apps

#### Example: Clock App

```cpp
class ClockApp : public DokiApp {
    void onCreate() override {
        _timeLabel = createValueLabel(getScreen(), "Syncing...",
                                      Colors::PRIMARY, -50);
    }

    void onUpdate() override {
        if (!TimeService::getInstance().isTimeSynced()) {
            lv_label_set_text(_timeLabel, "Syncing...");
            return;
        }

        // Get time and update display
        String timeStr = TimeService::getInstance().getTimeString();
        lv_label_set_text(_timeLabel, timeStr.c_str());
    }
};
```

#### Example: Weather App

```cpp
void onUpdate() override {
    if (TimeService::getInstance().isTimeSynced()) {
        struct tm time = TimeService::getInstance().getLocalTime();

        // Show last update time
        String lastUpdate = String(time.tm_hour) + ":" +
                          String(time.tm_min);
        lv_label_set_text(_updateLabel, lastUpdate.c_str());
    }
}
```

### 5. Using in JavaScript (via js_engine.cpp)

JavaScript apps can access time via the `getTime()` function:

```javascript
function updateClock() {
    var time = getTime();

    if (time === null) {
        print("Time not synced yet");
        return;
    }

    print("Hour: " + time.hour);      // 0-23
    print("Minute: " + time.minute);  // 0-59
    print("Second: " + time.second);  // 0-59
    print("Day: " + time.day);        // 1-31
    print("Month: " + time.month);    // 1-12
    print("Year: " + time.year);      // Full year
}
```

The JavaScript binding implementation in `js_engine.cpp`:

```cpp
duk_ret_t JSEngine::_js_getTime(duk_context* ctx) {
    TimeService& timeService = TimeService::getInstance();

    // Check if synced
    if (!timeService.isTimeSynced()) {
        duk_push_null(ctx);
        return 1;
    }

    // Get time
    struct tm timeInfo = timeService.getLocalTime();
    if (timeInfo.tm_year == 0) {
        duk_push_null(ctx);
        return 1;
    }

    // Build JavaScript object
    duk_push_object(ctx);
    duk_push_int(ctx, timeInfo.tm_hour);
    duk_put_prop_string(ctx, -2, "hour");
    duk_push_int(ctx, timeInfo.tm_min);
    duk_put_prop_string(ctx, -2, "minute");
    // ... other fields

    return 1;
}
```

## Best Practices

### DO ✅

1. **Always check sync status** before using time
   ```cpp
   if (TimeService::getInstance().isTimeSynced()) {
       // Use time
   }
   ```

2. **Initialize once** in main.cpp after WiFi is connected

3. **Use the singleton** - never create your own NTPClient
   ```cpp
   TimeService::getInstance().begin();  // GOOD
   ```

4. **Show "Syncing..." UI** while waiting for sync
   ```cpp
   if (!synced) {
       lv_label_set_text(label, "Syncing...");
   }
   ```

5. **Validate time data** before using
   ```cpp
   struct tm time = timeService.getLocalTime();
   if (time.tm_year > 0) {  // Valid
       // Use time
   }
   ```

### DON'T ❌

1. **Don't create your own NTPClient**
   ```cpp
   NTPClient* client = new NTPClient(...);  // BAD!
   ```

2. **Don't call begin() multiple times**
   ```cpp
   TimeService::getInstance().begin();  // Once in main.cpp
   TimeService::getInstance().begin();  // BAD - don't do this!
   ```

3. **Don't block waiting for sync**
   ```cpp
   while (!timeService.isTimeSynced()) {
       delay(100);  // BAD - freezes everything!
   }
   ```

4. **Don't access NTP from multiple threads** (except via TimeService)
   ```cpp
   // TimeService handles thread safety, don't bypass it
   ```

5. **Don't use time without validation**
   ```cpp
   struct tm time = timeService.getLocalTime();
   int hour = time.tm_hour;  // BAD - might be invalid!
   ```

## Configuration

All timing constants are defined in [include/timing_constants.h](../include/timing_constants.h):

```cpp
// NTP sync intervals
#define UPDATE_INTERVAL_NTP_MS 3600000        // 1 hour when synced
#define UPDATE_INTERVAL_NTP_RETRY_MS 60000    // 1 minute on failure

// Delays
#define DELAY_NTP_INIT_MS 2000                // Wait before first sync

// NTP server (can be changed)
#define NTP_SERVER "pool.ntp.org"

// Timezone offset (configured in TimeService)
#define TIMEZONE_OFFSET_SECONDS 19800         // UTC+5:30 (India)
```

To change timezone:
```cpp
// In time_service.h constructor
_ntpClient = new NTPClient(*_udp, "pool.ntp.org",
                          19800,  // ← Change this (seconds from UTC)
                          UPDATE_INTERVAL_NTP_RETRY_MS);
```

## Troubleshooting

### "Time not synced" persists

**Symptom**: `isTimeSynced()` returns false for >1 minute

**Possible Causes**:
1. WiFi not connected - Check `WiFi.status() == WL_CONNECTED`
2. Firewall blocking NTP (port 123 UDP) - Test with other devices
3. NTP server unreachable - Try different server (time.google.com)
4. Router blocking outbound UDP - Check router settings

**Debug**:
```cpp
Serial.print("WiFi: ");
Serial.println(WiFi.status() == WL_CONNECTED ? "Connected" : "Disconnected");
Serial.print("Time synced: ");
Serial.println(TimeService::getInstance().isTimeSynced() ? "Yes" : "No");
```

### Time is wrong timezone

**Symptom**: Time is correct but in wrong timezone

**Solution**: Change timezone offset in TimeService initialization:
```cpp
// Asia/Kolkata (UTC+5:30) = 19800 seconds
_ntpClient = new NTPClient(*_udp, "pool.ntp.org", 19800, ...);

// US Pacific (UTC-8) = -28800 seconds
_ntpClient = new NTPClient(*_udp, "pool.ntp.org", -28800, ...);

// UK (UTC+0) = 0 seconds
_ntpClient = new NTPClient(*_udp, "pool.ntp.org", 0, ...);
```

### UDP DNS errors still appearing

**Symptom**: `[WiFiUdp.cpp:172] could not get host from dns`

**Possible Causes**:
1. Another part of code creating NTPClient - Search codebase for `new NTPClient`
2. TimeService.begin() called multiple times - Should be called once
3. Old clock app still using its own NTP - Migrate to TimeService

**Solution**: Ensure only TimeService creates NTPClient

### Background task crashed

**Symptom**: Time freezes at specific value

**Debug**:
```cpp
// Check if background task is running
TaskHandle_t handle = /* get handle somehow */;
if (eTaskGetState(handle) == eDeleted) {
    Serial.println("TimeService task crashed!");
}
```

**Solution**: Check for stack overflow - increase `TASK_STACK_NTP_SYNC` in hardware_config.h

## Migration Guide

### Migrating Existing App from Own NTP to TimeService

**Before** (old clock_app.h):
```cpp
class ClockApp : public DokiApp {
    WiFiUDP* _udp;
    NTPClient* _ntpClient;
    TaskHandle_t _ntpTaskHandle;

    void onCreate() override {
        _udp = new WiFiUDP();
        _ntpClient = new NTPClient(*_udp, "pool.ntp.org", 19800, 60000);
        _ntpClient->begin();
        xTaskCreate(ntpUpdateTask, ...);
    }

    void onUpdate() override {
        time_t epochTime = _ntpClient->getEpochTime();
        struct tm* timeInfo = localtime(&epochTime);
        // Use timeInfo->tm_hour, etc.
    }

    void onDestroy() override {
        vTaskDelete(_ntpTaskHandle);
        delete _ntpClient;
        delete _udp;
    }
};
```

**After** (using TimeService):
```cpp
class ClockApp : public DokiApp {
    // No NTP members needed!

    void onCreate() override {
        // No NTP initialization needed!
    }

    void onUpdate() override {
        if (!TimeService::getInstance().isTimeSynced()) {
            // Show "Syncing..."
            return;
        }

        struct tm timeInfo = TimeService::getInstance().getLocalTime();
        // Use timeInfo.tm_hour, etc. (note: not pointer!)
    }

    void onDestroy() override {
        // No NTP cleanup needed!
    }
};
```

**Benefits**:
- 85 lines removed
- No background task management
- No memory allocation/deallocation
- No UDP conflicts
- Simpler code

## Technical Details

### Memory Layout

```
TimeService (Singleton)
├─ NTPClient* (allocated once, never freed)
├─ WiFiUDP* (allocated once, never freed)
├─ TaskHandle_t (background task)
├─ volatile bool _synced (thread-safe flag)
└─ Total: ~4KB heap + 4KB task stack
```

### Task Priority

- **Task Name**: TimeService_Sync
- **Stack Size**: 4096 bytes (configurable via `TASK_STACK_NTP_SYNC`)
- **Priority**: 1 (low, background)
- **Core**: 0 (network core)
- **Why Core 0?**: Network operations should run on Core 0, freeing Core 1 for display rendering

### Thread Safety

- `_synced` flag is `volatile` for cross-thread visibility
- No mutex needed (single writer, multiple readers of boolean)
- NTPClient internally thread-safe for read operations
- Time reads are atomic (struct tm copy, not pointer)

### Error Suppression

To avoid console spam during sync attempts:
```cpp
esp_log_level_set("WiFiUdp", ESP_LOG_ERROR);
```

This suppresses INFO/WARNING logs but keeps ERROR logs for critical issues.

## Examples

### Complete Clock App Example

```cpp
#include "doki/app_base.h"
#include "doki/time_service.h"
#include "doki/lvgl_helpers.h"
#include "timing_constants.h"

using namespace Doki;

class ClockApp : public DokiApp {
private:
    lv_obj_t* _timeLabel;
    lv_obj_t* _dateLabel;
    lv_obj_t* _statusLabel;
    uint32_t _lastUpdate = 0;

public:
    void onCreate() override {
        // Create UI
        _timeLabel = createValueLabel(getScreen(), "--:--:--",
                                      Colors::PRIMARY, -40);
        _dateLabel = createStatusLabel(getScreen(), "--/--/----",
                                       Colors::DARK_GRAY, 0);
        _statusLabel = createStatusLabel(getScreen(), "Syncing...",
                                         Colors::WARNING, 40);
    }

    void onStart() override {
        // Nothing to start - TimeService already running
    }

    void onUpdate() override {
        uint32_t now = millis();
        if (now - _lastUpdate < UPDATE_INTERVAL_CLOCK_MS) return;
        _lastUpdate = now;

        if (!TimeService::getInstance().isTimeSynced()) {
            lv_label_set_text(_timeLabel, "--:--:--");
            lv_label_set_text(_dateLabel, "--/--/----");
            lv_label_set_text(_statusLabel, "Syncing...");
            return;
        }

        // Update time
        String timeStr = TimeService::getInstance().getTimeString();
        lv_label_set_text(_timeLabel, timeStr.c_str());

        // Update date
        String dateStr = TimeService::getInstance().getDateString();
        lv_label_set_text(_dateLabel, dateStr.c_str());

        // Update status
        lv_label_set_text(_statusLabel, "Time synced");
    }

    void onDestroy() override {
        // LVGL auto-cleans UI elements
    }
};
```

### Complete JavaScript Clock Example

```javascript
// clock.js
var lastHour = -1;

function setup() {
    print("Clock app started");
}

function update() {
    var time = getTime();

    if (time === null) {
        print("Waiting for time sync...");
        return;
    }

    // Only print when hour changes (reduce spam)
    if (time.hour !== lastHour) {
        print("Time: " +
              time.hour + ":" +
              time.minute + ":" +
              time.second);
        lastHour = time.hour;
    }
}
```

## Summary

**TimeService** provides:
- ✅ Centralized time management
- ✅ Zero UDP conflicts
- ✅ Background synchronization
- ✅ Thread-safe access
- ✅ Simple API
- ✅ Memory efficient
- ✅ Production ready

**Use it for**:
- Clock displays
- Timestamp logs
- Scheduling events
- Time-based logic
- Any app needing current time

**Remember**:
1. Initialize once in main.cpp
2. Always check `isTimeSynced()` first
3. Never create your own NTPClient
4. Use the singleton: `TimeService::getInstance()`

---

**See also**:
- [System Architecture](./03-system-architecture.md)
- [Application Framework](./04-application-framework.md)
- [hardware_config.h](../include/hardware_config.h)
- [timing_constants.h](../include/timing_constants.h)
