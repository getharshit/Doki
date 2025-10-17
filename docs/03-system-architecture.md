# System Architecture

This document describes the overall architecture of Doki OS, including the main components, data flow, and design principles.

## High-Level Architecture

```
┌───────────────────────────────────────────────────────────────┐
│                      User Interface Layer                      │
│  ┌─────────────────────┐         ┌─────────────────────────┐  │
│  │  Display 0          │         │  Display 1              │  │
│  │  (240x320 ST7789)   │         │  (240x320 ST7789)       │  │
│  │  Running: ClockApp  │         │  Running: WeatherApp    │  │
│  └─────────────────────┘         └─────────────────────────┘  │
│               ↕                            ↕                   │
│  ┌─────────────────────────────────────────────────────────┐  │
│  │           Web Dashboard (HTTP Interface)                │  │
│  │     Browser → API Calls → Control Both Displays         │  │
│  └─────────────────────────────────────────────────────────┘  │
└───────────────────────────────────────────────────────────────┘
                           ↓
┌───────────────────────────────────────────────────────────────┐
│                    Application Layer                           │
│  ┌──────────┐  ┌──────────┐  ┌──────────┐  ┌──────────┐     │
│  │ ClockApp │  │WeatherApp│  │ SysInfo  │  │ Blank    │ ... │
│  └──────────┘  └──────────┘  └──────────┘  └──────────┘     │
│         ↑             ↑             ↑             ↑            │
│  ┌─────────────────────────────────────────────────────────┐  │
│  │            DokiApp Base (Lifecycle Manager)             │  │
│  └─────────────────────────────────────────────────────────┘  │
└───────────────────────────────────────────────────────────────┘
                           ↓
┌───────────────────────────────────────────────────────────────┐
│                      Services Layer                            │
│  ┌─────────────┐  ┌─────────────┐  ┌─────────────────────┐  │
│  │ Weather     │  │ Time/NTP    │  │ HTTP Server         │  │
│  │ Service     │  │ Manager     │  │ (ESPAsyncWebServer) │  │
│  └─────────────┘  └─────────────┘  └─────────────────────┘  │
└───────────────────────────────────────────────────────────────┘
                           ↓
┌───────────────────────────────────────────────────────────────┐
│                Hardware Abstraction Layer                      │
│  ┌─────────────┐  ┌─────────────┐  ┌─────────────────────┐  │
│  │ ST7789      │  │ LVGL        │  │ WiFi                │  │
│  │ SPI Driver  │  │ Graphics    │  │ (AP + Station)      │  │
│  └─────────────┘  └─────────────┘  └─────────────────────┘  │
└───────────────────────────────────────────────────────────────┘
```

## Core Components

### 0. Configuration System

**Purpose**: Centralized hardware and timing configuration

**Location**:
- [include/hardware_config.h](../include/hardware_config.h) - Hardware constants
- [include/timing_constants.h](../include/timing_constants.h) - Timing values

**Why Centralized Configuration**:
- Single source of truth for all constants
- Easy board adaptation (change pins in one place)
- Consistent timing across system
- Eliminates scattered magic numbers

**Hardware Config Examples**:
```cpp
// Display pins
#define DISPLAY_0_CS_PIN 33
#define DISPLAY_0_DC_PIN 15
#define DISPLAY_WIDTH 240
#define DISPLAY_HEIGHT 320

// Memory configuration
#define JS_HEAP_SIZE_KB 128
#define LVGL_BUFFER_LINES 40

// Task configuration
#define TASK_STACK_NTP_SYNC 4096
#define TASK_PRIORITY_NTP 1
#define TASK_CORE_NETWORK 0
```

**Timing Config Examples**:
```cpp
// Update intervals
#define UPDATE_INTERVAL_CLOCK_MS 1000
#define UPDATE_INTERVAL_NTP_MS 3600000      // 1 hour
#define UPDATE_INTERVAL_NTP_RETRY_MS 60000  // 1 minute
#define UPDATE_INTERVAL_WEATHER_MS 600000   // 10 minutes

// Timeouts and delays
#define TIMEOUT_HTTP_REQUEST_MS 5000
#define DELAY_NTP_INIT_MS 2000
```

### 1. Main Program ([src/main.cpp](../src/main.cpp))

**Purpose**: Entry point and system coordinator

**Responsibilities**:
- Initialize hardware (SPI, displays, WiFi)
- Set up LVGL graphics system
- Configure HTTP web server and API endpoints
- Manage dual display contexts
- Initialize TimeService for system-wide time sync
- Main loop: Update apps and render displays

**Key Functions**:
```cpp
void setup() {
    // 1. Initialize hardware
    // 2. Initialize LVGL
    // 3. Initialize dual displays
    // 4. Set up WiFi (hybrid AP+STA mode)
    // 5. Start HTTP server
    // 6. Load default apps
}

void loop() {
    // 1. Update all running apps
    // 2. Run LVGL timer handler (rendering)
    // 3. Yield to FreeRTOS
}
```

**Design Pattern**: Procedural with object management

### 2. Display Management

#### Display Structure

Each display is represented by a structure containing:

```cpp
struct Display {
    uint8_t id;                     // Display ID (0 or 1)
    uint8_t cs_pin, dc_pin, rst_pin; // Hardware pins

    lv_disp_draw_buf_t draw_buf;    // LVGL double-buffer
    lv_color_t* buf1;               // Buffer 1 (PSRAM)
    lv_color_t* buf2;               // Buffer 2 (PSRAM)
    lv_disp_drv_t disp_drv;         // LVGL display driver
    lv_disp_t* disp;                // LVGL display object

    Doki::DokiApp* app;             // Currently running app
    String appId;                   // App identifier
    uint32_t appStartTime;          // App start timestamp
};
```

#### Display Initialization Flow

```
initDisplay(id, cs, dc, rst)
    ↓
Initialize ST7789 hardware
    ↓
Allocate PSRAM buffers (buf1, buf2)
    ↓
Initialize LVGL draw buffer
    ↓
Register LVGL display driver
    ↓
Set flush callback (lvgl_flush_display0/1)
    ↓
Return display object
```

#### Flush Callbacks

Each display has a dedicated flush callback that writes pixel data to the ST7789 via SPI:

```cpp
void lvgl_flush_display0(lv_disp_drv_t* disp,
                         const lv_area_t* area,
                         lv_color_t* color_p) {
    // Set drawing window
    setAddrWindow(DISP0_CS, DISP0_DC, x1, y1, x2, y2);

    // Write pixel data via SPI
    for (uint32_t i = 0; i < w * h; i++) {
        spi.transfer16(color_p[i].full);
    }

    // Notify LVGL rendering complete
    lv_disp_flush_ready(disp);
}
```

### 3. Application Framework

#### DokiApp Base Class ([include/doki/app_base.h](../include/doki/app_base.h))

**Purpose**: Abstract base class for all apps

**Lifecycle States**:
```
IDLE → CREATED → STARTED → PAUSED → DESTROYED
```

**Lifecycle Methods** (must be implemented by subclasses):

```cpp
class DokiApp {
    virtual void onCreate() = 0;   // Create UI, initialize variables
    virtual void onStart() = 0;    // Start animations, timers
    virtual void onUpdate() = 0;   // Called every frame
    virtual void onPause() {};     // Pause (optional)
    virtual void onDestroy() = 0;  // Clean up resources
};
```

**Helper Methods**:
- `getScreen()`: Get LVGL screen object
- `clearScreen()`: Remove all UI elements
- `log(message)`: Print debug message
- `getUptime()`: Get app runtime in milliseconds

#### App Loading Flow

```
loadAppOnDisplay(displayId, appId)
    ↓
Set LVGL context for display
    ↓ lv_disp_set_default(display[id].disp)
    ↓
Clear screen FIRST
    ↓ lv_obj_clean(screen)  ← Prevents dangling pointers
    ↓
Destroy old app (if any)
    ↓ app->onDestroy()
    ↓ delete app
    ↓
Create new app instance
    ↓ createApp(appId) → new ClockApp()
    ↓
Initialize app on clean screen
    ↓ app->onCreate()   (Create UI)
    ↓ app->onStart()    (Start app)
    ↓
Store app reference in Display structure
```

**Critical**: The screen is cleaned **before** destroying the old app to prevent race conditions where LVGL's task handler might trigger callbacks (`onUpdate()`) on deleted UI objects, which would cause null pointer crashes.

### 4. HTTP Server and Web Dashboard

#### SimpleHttpServer Module

**Location**: [include/doki/simple_http_server.h](../include/doki/simple_http_server.h), [src/doki/simple_http_server.cpp](../src/doki/simple_http_server.cpp)

**Purpose**: Decoupled HTTP server for web dashboard control

**Architecture**: Callback-based design to avoid tight coupling with main application logic

```cpp
class SimpleHttpServer {
public:
    static bool begin(uint16_t port = 80);
    static void setLoadAppCallback(bool (*callback)(uint8_t, const String&));
    static void setStatusCallback(void (*callback)(uint8_t, String&, uint32_t&));
};
```

**Integration** (in main.cpp):
```cpp
// Set callbacks
SimpleHttpServer::setLoadAppCallback(loadAppOnDisplay);
SimpleHttpServer::setStatusCallback(getDisplayStatus);

// Start server
SimpleHttpServer::begin(80);
```

#### Server Endpoints

| Endpoint | Method | Purpose |
|----------|--------|---------|
| `/` | GET | Web dashboard HTML |
| `/api/apps` | GET | Get list of available apps |
| `/api/load` | POST | Load app on display (`?display=0&app=clock`) |
| `/api/status` | GET | Get display status and system info |

#### API Request Flow

```
Browser → POST /api/load?display=0&app=clock
    ↓
SimpleHttpServer::handleLoadApp(request)
    ↓
Parse parameters (displayId, appId)
    ↓
Call _loadAppCallback(0, "clock")  ← Callback to main.cpp
    ↓
main.cpp: loadAppOnDisplay(0, "clock")
    ↓ (See App Loading Flow above)
    ↓
Return JSON response:
{
    "success": true,
    "message": "Loaded clock on Display 0"
}
```

#### Dashboard Architecture

The dashboard is a single-page HTML application embedded in SimpleHttpServer module:

**Features**:
- Real-time display status (auto-refresh every 3s)
- One-click app loading per display
- Display selector (switch between Display 0 and Display 1)
- Available apps grid with instant loading
- Responsive design (works on mobile and desktop)

**Technology Stack**:
- Pure HTML/CSS/JavaScript (no frameworks, no template literals)
- Fetch API for HTTP requests
- Flexbox layout for responsive design
- String concatenation instead of template literals (ES6 compatibility in C++ rawliteral strings)

**Example JavaScript** (avoiding template literals):
```javascript
function loadApp(appId) {
    const url = '/api/load?display=' + selectedDisplay + '&app=' + appId;
    fetch(url, { method: 'POST' })
        .then(function(res) { return res.json(); })
        .then(function(data) {
            if (data.success) {
                showMessage('Loaded ' + appId + ' on Display ' + selectedDisplay, 'success');
            }
        });
}
```

### 5. UI Helper Library

**Purpose**: Reusable LVGL UI creation functions

**Location**: [include/doki/lvgl_helpers.h](../include/doki/lvgl_helpers.h)

**Benefits**:
- Reduces app code by 30-40%
- Consistent styling across apps
- Less boilerplate for common UI patterns
- Easier to maintain UI code

**Available Functions**:
```cpp
// Label creation helpers
lv_obj_t* createTitleLabel(parent, text, color, y_offset);
lv_obj_t* createValueLabel(parent, text, color, y_offset);
lv_obj_t* createStyledLabel(parent, text, color, font, align, x, y);
lv_obj_t* createStatusLabel(parent, text, color, y_offset);
lv_obj_t* createMetricLabel(parent, text, y_offset);
lv_obj_t* createLargeValueLabel(parent, text, color, y_offset);

// Progress bar helpers
lv_obj_t* createProgressBar(parent, width, height, bg, fg, align, x, y);
lv_obj_t* createHorizontalProgressBar(parent, width, y_offset);

// Container helpers
lv_obj_t* createContainer(parent, width, height, align, x, y);
lv_obj_t* createCenteredContainer(parent, width, height);

// Image helpers
lv_obj_t* createCenteredImage(parent, src);
lv_obj_t* createImageWithOffset(parent, src, x, y);

// Style helpers
void applyCardStyle(obj, bg_color, border_color);
void applyGlowEffect(obj, color, intensity);
void applyRoundedCorners(obj, radius);
```

**Color Palette**:
```cpp
namespace Colors {
    constexpr uint32_t PRIMARY = 0x667eea;
    constexpr uint32_t SUCCESS = 0x10b981;
    constexpr uint32_t WARNING = 0xf59e0b;
    constexpr uint32_t ERROR = 0xff4444;
    constexpr uint32_t INFO = 0x3b82f6;
    constexpr uint32_t ORANGE = 0xff6b35;
    constexpr uint32_t CYAN = 0x00d4ff;
    constexpr uint32_t PURPLE = 0x9333ea;
    constexpr uint32_t LIGHT_GRAY = 0xe5e7eb;
    constexpr uint32_t DARK_GRAY = 0x6b7280;
}
```

**Example - Before and After**:
```cpp
// Before (7 lines)
_timeLabel = lv_label_create(getScreen());
lv_label_set_text(_timeLabel, "12:34:56");
lv_obj_align(_timeLabel, LV_ALIGN_CENTER, 0, -50);
lv_obj_set_style_text_font(_timeLabel, &lv_font_montserrat_48, 0);
lv_obj_set_style_text_color(_timeLabel, lv_color_hex(0x667eea), 0);

// After (1 line)
_timeLabel = createValueLabel(getScreen(), "12:34:56", Colors::PRIMARY, -50);
```

### 6. Services Layer

#### Weather Service ([src/doki/weather_service.cpp](../src/doki/weather_service.cpp))

**Purpose**: Fetch weather data from WeatherAPI.com

**Architecture**:
```cpp
class WeatherService {
    static String _apiKey;
    static WeatherData _cachedData;
    static const uint32_t CACHE_DURATION_MS = 600000;  // 10 min

    static bool getCurrentWeather(location, data) {
        if (isCacheValid()) return cached data;
        return _fetchWeather(location, data);
    }
};
```

**API Call Flow**:
```
WeatherService::getCurrentWeather("Mumbai", data)
    ↓
Check cache (valid for 10 minutes)
    ↓ (if valid) Return cached data
    ↓ (if expired) Fetch new data
    ↓
HTTPClient → GET weatherapi.com/current.json
    ↓
Parse JSON response (ArduinoJson)
    ↓
Extract: temp, condition, humidity, wind
    ↓
Store in cache
    ↓
Return data
```

#### TimeService (NTP Client Singleton)

**Purpose**: Centralized time synchronization for entire system

**Architecture**: Singleton pattern with background synchronization

**Location**: [include/doki/time_service.h](../include/doki/time_service.h)

**Key Features**:
- **Single NTP Client**: One client for entire system (eliminates UDP conflicts)
- **Background Task**: Non-blocking synchronization on Core 0
- **Smart Retry**: 1-hour interval when synced, 1-minute retry on failure
- **Thread-Safe**: Volatile flags for cross-task communication
- **Error Suppression**: Reduces console spam during sync attempts

**Implementation**:
```cpp
class TimeService {
public:
    static TimeService& getInstance();  // Singleton access
    bool begin();                       // Initialize and start sync task
    bool isTimeSynced() const;         // Check sync status
    struct tm getLocalTime();          // Get current time
    String getTimeString();            // Formatted time string
    String getDateString();            // Formatted date string
};
```

**Usage Example**:
```cpp
// Initialize once in main.cpp
TimeService::getInstance().begin();

// Use anywhere in the system
if (TimeService::getInstance().isTimeSynced()) {
    struct tm time = TimeService::getInstance().getLocalTime();
    String timeStr = TimeService::getInstance().getTimeString();
}
```

**Background Task**:
```cpp
static void syncTask(void* parameter) {
    vTaskDelay(pdMS_TO_TICKS(DELAY_NTP_INIT_MS));

    // Initial sync
    if (_ntpClient->forceUpdate()) {
        _synced = true;
    }

    // Periodic updates
    while (true) {
        vTaskDelay(_synced ?
            pdMS_TO_TICKS(UPDATE_INTERVAL_NTP_MS) :      // 1 hour
            pdMS_TO_TICKS(UPDATE_INTERVAL_NTP_RETRY_MS)  // 1 minute
        );
        if (_ntpClient->update()) {
            _synced = true;
        }
    }
}
```

**Benefits Over Previous Approach**:
- ✅ No duplicate NTP clients (eliminates UDP DNS errors)
- ✅ Consistent time across all apps
- ✅ Single background task (saves ~10KB RAM)
- ✅ Centralized configuration
- ✅ Easy to use from any app or JavaScript

### 6. Hardware Abstraction Layer

#### ST7789 Driver

**Location**: Inline in [src/main.cpp](../src/main.cpp)

**Low-level Functions**:
```cpp
void writeCommand(cs, dc, cmd);       // Send command byte
void writeData(cs, dc, data);         // Send data byte
void writeData16(cs, dc, data);       // Send 16-bit data
void setAddrWindow(cs, dc, x0, y0, x1, y1);  // Set drawing area
void initDisplayHardware(cs, dc, rst); // Initialize ST7789
```

**Initialization Sequence**:
```
Hardware Reset (RST pin)
    ↓
SWRESET (Software Reset)
    ↓
SLPOUT (Sleep Out)
    ↓
COLMOD (Color Mode: RGB565)
    ↓
MADCTL (Display Orientation)
    ↓
NORON (Normal Display On)
    ↓
DISPON (Display On)
```

#### LVGL Integration

**Buffer Allocation**:
- Two buffers per display (double buffering)
- Each buffer: 240px × 40 lines × 2 bytes = 19,200 bytes
- Total per display: ~38KB (allocated in PSRAM)

**Rendering Flow**:
```
App updates UI → LVGL marks dirty areas
    ↓
LVGL renders to buffer
    ↓
LVGL calls flush_cb (lvgl_flush_display0/1)
    ↓
Flush callback writes to ST7789 via SPI
    ↓
Display updates
```

### 7. WiFi Hybrid Mode

**Purpose**: Simultaneous Access Point (for dashboard) and Station mode (for internet)

**Configuration**:
```cpp
WiFi.mode(WIFI_AP_STA);  // Enable both modes

// Connect to internet (Station mode)
WiFi.begin(STATION_SSID, STATION_PASSWORD);

// Create Access Point
WiFi.softAP(AP_SSID, AP_PASSWORD);
```

**Network Layout**:
```
Internet
    ↓
Home WiFi Router
    ↓
ESP32-S3 (Station Mode)
    ├─ IP: 192.168.1.100 (from router)
    ├─ Access Weather API
    └─ Access NTP servers

ESP32-S3 (AP Mode)
    ├─ IP: 192.168.4.1 (default AP IP)
    ├─ SSID: DokiOS-Control
    └─ Serves Dashboard
         ↓
    Client devices
    (Connect to DokiOS-Control)
```

## Design Principles

### 1. Non-Blocking Architecture

**Problem**: Blocking operations freeze the entire system

**Solution**:
- NTP updates run in separate FreeRTOS tasks
- HTTP server is async (ESPAsyncWebServer)
- Apps use time-based updates (not blocking delays)

**Example**:
```cpp
// BAD: Blocking
void onUpdate() {
    delay(1000);  // Freezes everything!
    updateDisplay();
}

// GOOD: Non-blocking
void onUpdate() {
    uint32_t now = millis();
    if (now - lastUpdate >= 1000) {
        updateDisplay();
        lastUpdate = now;
    }
}
```

### 2. Memory Management

**PSRAM Usage**:
- LVGL display buffers (~76KB for both displays)
- Large data structures (weather cache, etc.)
- Assets (embedded images)

**Heap Usage**:
- LVGL internal memory pool (64KB)
- App instances and UI objects
- Network buffers

**Best Practices**:
- Use `heap_caps_malloc(size, MALLOC_CAP_SPIRAM)` for large buffers
- Clean up resources in `onDestroy()`
- Monitor memory with System Info app

### 3. Thread Safety

**LVGL Mutex**:
```cpp
SemaphoreHandle_t lvgl_mutex;

// When loading app
xSemaphoreTake(lvgl_mutex, portMAX_DELAY);
app->onCreate();  // LVGL operations
xSemaphoreGive(lvgl_mutex);
```

**Why**: LVGL is not thread-safe. Protect UI operations from concurrent access.

### 4. Modularity

**App Independence**:
- Each app is self-contained
- Apps don't depend on each other
- Easy to add/remove apps

**Service Abstraction**:
- Services provide clean APIs (WeatherService, etc.)
- Apps use services without knowing implementation details

### 5. Resource Cleanup

**App Lifecycle**:
```cpp
void onDestroy() override {
    // Stop timers
    if (_ntpTaskHandle) vTaskDelete(_ntpTaskHandle);

    // Free allocated memory
    delete _ntpClient;
    delete _udp;

    // LVGL auto-cleans UI elements
}
```

## Data Flow Examples

### Example 1: User Loads Clock App

```
1. User clicks "Clock" button in dashboard
2. Browser sends: POST /api/display/load?display=0&app=clock
3. ESP32 receives request in handleLoadAppOnDisplay()
4. Call loadAppOnDisplay(0, "clock")
5. Unload current app on Display 0 (if any)
6. Create ClockApp instance
7. Set LVGL context to Display 0
8. Call clockApp->onCreate() → Create UI elements
9. Call clockApp->onStart() → Start NTP task
10. Store app reference in displays[0].app
11. Return success JSON to browser
12. Browser refreshes status, shows "Clock" on Display 0
```

### Example 2: Clock App Updates Time

```
1. Main loop calls displays[0].app->onUpdate()
2. ClockApp::onUpdate() checks if 1 second elapsed
3. If yes, call updateDisplay()
4. Read time from NTPClient (from background task)
5. Format time string (HH:MM:SS)
6. Update LVGL label: lv_label_set_text(_timeLabel, "12:34:56")
7. LVGL marks label as dirty
8. In next lv_timer_handler():
   - LVGL renders dirty area to buffer
   - Calls lvgl_flush_display0()
   - Writes pixels to ST7789 via SPI
9. Display shows updated time
```

### Example 3: Weather App Fetches Data

```
1. WeatherApp::onStart() calls fetchWeather()
2. Call WeatherService::getCurrentWeather("Mumbai", data)
3. Check if cached data is valid (< 10 min old)
4. If expired:
   a. Create HTTPClient
   b. Connect to weatherapi.com
   c. Send GET request: /v1/current.json?q=Mumbai&key=...
   d. Receive JSON response
   e. Parse with ArduinoJson
   f. Extract: tempC, condition, humidity, wind
   g. Store in cache
5. Return weather data
6. WeatherApp updates UI with new data
7. LVGL renders updated display
```

## Performance Characteristics

### Update Rates

| Component | Update Rate | Notes |
|-----------|-------------|-------|
| Main Loop | ~200 FPS | Calls app updates and LVGL handler |
| App onUpdate() | 30-60 FPS | Apps self-throttle to 1-10 FPS |
| LVGL Rendering | 30 FPS | Configurable via LV_DISP_DEF_REFR_PERIOD |
| NTP Sync | Every 60s | Background task |
| Weather Fetch | Every 10m | Cached to reduce API calls |
| Dashboard Refresh | Every 2s | Auto-refresh via JavaScript |

### Memory Usage

| Component | Heap | PSRAM |
|-----------|------|-------|
| LVGL Internal | 64KB | - |
| Display Buffers | - | 76KB |
| Weather Cache | 2KB | - |
| HTTP Server | 30KB | - |
| Apps (varies) | 5-20KB | - |
| **Total Typical** | ~150KB | ~400KB |

## Next Steps

- [Application Framework](./04-application-framework.md) - Deep dive into app development
- [Display Management](./05-display-management.md) - LVGL and display handling
- [Services](./06-services.md) - Weather, time, and HTTP services

---

[← Back: Platform Configuration](./02-platform-configuration.md) | [Next: Application Framework →](./04-application-framework.md)
