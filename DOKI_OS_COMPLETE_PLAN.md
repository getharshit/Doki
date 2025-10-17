# Doki OS - Complete Architecture & Implementation Plan

## Executive Summary

**Project**: Doki OS - Open Platform for ESP32 Smart Displays
**Vision**: Build a lightweight, extensible operating system that enables dynamic app loading, multi-display support, and remote control via mobile applications.
**Current Status**: MVP with working apps, but needs architectural refactoring
**Timeline**: 14 weeks total (4 weeks refactoring + 10 weeks new features)
**Team Size**: 1 developer (modular approach for scalability)

---

## Table of Contents

1. [Current State Analysis](#1-current-state-analysis)
2. [System Architecture](#2-system-architecture)
3. [Critical Architectural Improvements](#3-critical-architectural-improvements)
4. [Refactoring Strategy](#4-refactoring-strategy)
5. [Component Design](#5-component-design)
6. [Mobile Application](#6-mobile-application)
7. [Development Phases](#7-development-phases)
8. [API Specification](#8-api-specification)
9. [Security & Safety](#9-security--safety)
10. [Testing Strategy](#10-testing-strategy)
11. [Success Metrics](#11-success-metrics)

---

## 1. Current State Analysis

### 1.1 What We Already Have (Excellent Foundation!)

**✅ Fully Implemented Managers (Not Currently Used)**:

1. **AppManager** (`include/doki/app_manager.h`, `src/doki/app_manager.cpp` - 275 lines)
   - ✅ App registry with factory pattern
   - ✅ App lifecycle management (load/unload)
   - ✅ getCurrentApp(), getRegisteredApps(), etc.
   - ✅ Memory and task scheduler integration
   - **STATUS**: Ready to use, needs multi-display support

2. **EventSystem** (`include/doki/event_system.h`, `src/doki/event_system.cpp` - 140 lines)
   - ✅ Pub/Sub messaging
   - ✅ subscribe(), publish() API
   - ✅ Predefined event types (WIFI_CONNECTED, APP_LOADED, etc.)
   - **STATUS**: Complete and ready to activate

3. **MemoryManager** (`include/doki/memory_manager.h`, `src/doki/memory_manager.cpp` - 227 lines)
   - ✅ Per-app memory tracking
   - ✅ Memory leak detection
   - ✅ System memory reporting
   - **STATUS**: Complete and ready to activate

4. **DisplayManager** (`include/doki/display_manager.h`, `src/doki/display_manager.cpp` - 409 lines)
   - ✅ Multi-display initialization
   - ✅ ST7789 driver integration
   - **STATUS**: Partially used, needs integration review

5. **Other Infrastructure**:
   - `task_scheduler.h/cpp` - FreeRTOS task management
   - `storage_manager.h/cpp` - NVS for WiFi credentials
   - `api_client.h/cpp` - HTTP client for external APIs
   - `qr_generator.h/cpp` - QR code generation

**✅ Active Services (Currently Working)**:
- `simple_http_server.h/cpp` - HTTP server with dashboard
- `filesystem_manager.h/cpp` - SPIFFS file operations
- `media_service.h/cpp` - Media upload/preview
- `lvgl_fs_driver.h/cpp` - LVGL filesystem bridge
- `weather_service.h/cpp` - Weather API integration
- `setup_portal.h/cpp` - WiFi setup mode
- All apps in `src/apps/` (8 working apps)

### 1.2 Current Problems

**❌ Architectural Gaps**:
1. **No App Registry** - Apps hardcoded in `createApp()` function
2. **No Memory Quotas** - Apps can allocate unlimited memory
3. **No Sandboxing** - Native apps have full system access
4. **No State Persistence** - App state lost on switch
5. **No Event System** - Apps are isolated, can't communicate
6. **Tight Coupling** - Most logic in main.cpp (600+ lines)

**❌ Duplicate Files**:
1. `http_server.h/cpp` - Duplicate of `simple_http_server.h` (DELETE)
2. `wifi_manager.h/cpp` - May be unused (VERIFY THEN DELETE)

**⚠️ Active Issues**:
1. Media upload - PSRAM memory allocation for image decoders
2. Display selection bug in media uploads
3. LVGL render errors during app switching

### 1.3 The Opportunity

We have **excellent infrastructure already built** - we just need to:
1. ✅ Delete duplicates
2. ✅ Activate existing managers (AppManager, EventSystem, MemoryManager)
3. ✅ Create missing components (StatePersistence, SandboxManager, MemoryQuotaManager)
4. ✅ Refactor main.cpp to use managers
5. ✅ Update apps to use new features

**Result**: Transform from manual management to clean, modular architecture in 3-4 weeks!

---

## 2. System Architecture

### 2.1 High-Level Architecture

```
┌─────────────────────────────────────────────────────────────────┐
│                     ECOSYSTEM OVERVIEW                           │
└─────────────────────────────────────────────────────────────────┘

┌──────────────────┐         ┌──────────────────┐         ┌──────────────────┐
│                  │         │                  │         │                  │
│  Mobile App      │◄───────►│   Doki OS        │◄───────►│  Cloud Services  │
│  (iOS/Android)   │  HTTP   │   (ESP32-S3)     │  HTTPS  │  (Optional)      │
│                  │  REST   │                  │  REST   │                  │
└──────────────────┘         └──────────────────┘         └──────────────────┘
      │                            │                              │
      ▼                            ▼                              ▼
┌──────────────────┐         ┌──────────────────┐         ┌──────────────────┐
│ • Device Control │         │ • App Runtime    │         │ • App Repository │
│ • App Store UI   │         │ • Display Mgmt   │         │ • User Accounts  │
│ • Code Editor    │         │ • Network Stack  │         │ • Analytics      │
│ • Settings       │         │ • Storage Mgmt   │         │ • OTA Updates    │
└──────────────────┘         └──────────────────┘         └──────────────────┘
```

### 2.2 Doki OS Core Architecture (Target State)

```
┌─────────────────────────────────────────────────────────────────┐
│                         DOKI OS (ESP32-S3)                       │
├─────────────────────────────────────────────────────────────────┤
│  ┌────────────────────────────────────────────────────────┐     │
│  │              APPLICATION LAYER                          │     │
│  │  ┌──────────────┐  ┌──────────────┐  ┌─────────────┐ │     │
│  │  │ Native Apps  │  │ JS Apps      │  │ System Apps │ │     │
│  │  │ • Clock      │  │ • Weather    │  │ • Setup     │ │     │
│  │  │ • SysInfo    │  │ • Custom...  │  │ • OTA       │ │     │
│  │  └──────────────┘  └──────────────┘  └─────────────┘ │     │
│  └────────────────────────────────────────────────────────┘     │
│                              ↓                                   │
│  ┌────────────────────────────────────────────────────────┐     │
│  │               RUNTIME LAYER                             │     │
│  │  ┌─────────────────┐        ┌─────────────────┐       │     │
│  │  │  App Manager    │        │  JS Runtime     │       │     │
│  │  │  • Lifecycle    │        │  (Duktape)      │       │     │
│  │  │  • Registry     │◄──────►│  • Bindings     │       │     │
│  │  │  • Multi-Disp   │        │  • Sandbox      │       │     │
│  │  └─────────────────┘        └─────────────────┘       │     │
│  │  ┌─────────────────┐        ┌─────────────────┐       │     │
│  │  │ Display Manager │        │ Event System    │       │     │
│  │  │  • Multi-screen │◄──────►│  • Pub/Sub      │       │     │
│  │  │  • Transitions  │        │  • Message Queue│       │     │
│  │  └─────────────────┘        └─────────────────┘       │     │
│  └────────────────────────────────────────────────────────┘     │
│                              ↓                                   │
│  ┌────────────────────────────────────────────────────────┐     │
│  │              SERVICE LAYER                              │     │
│  │  ┌────────────┐  ┌────────────┐  ┌─────────────┐     │     │
│  │  │ Network    │  │ Storage    │  │ Resource    │     │     │
│  │  │ Manager    │  │ Manager    │  │ Manager     │     │     │
│  │  │ • HTTP Srv │  │ • SPIFFS   │  │ • Memory    │     │     │
│  │  │ • WiFi Mgr │  │ • NVS      │  │ • Tasks     │     │     │
│  │  │ • mDNS     │  │ • Media    │  │ • Quotas    │     │     │
│  │  └────────────┘  └────────────┘  └─────────────┘     │     │
│  └────────────────────────────────────────────────────────┘     │
│                              ↓                                   │
│  ┌────────────────────────────────────────────────────────┐     │
│  │              HARDWARE LAYER                             │     │
│  │  ESP32-S3 • 512KB SRAM • 2MB PSRAM • 16MB Flash       │     │
│  │  WiFi 2.4GHz • ST7789 Displays • USB-C                │     │
│  └────────────────────────────────────────────────────────┘     │
└─────────────────────────────────────────────────────────────────┘
```

---

## 3. Critical Architectural Improvements

### 3.1 App Registry System

**Current Problem**: Apps are hardcoded in `createApp()` - adding apps requires code changes.

**Solution**: Dynamic app registry with factory pattern (partially exists in AppManager!)

```cpp
class AppRegistry {
public:
    using AppFactory = std::function<DokiApp*(uint8_t displayId)>;

    // Registration
    bool registerApp(const AppMetadata& metadata, AppFactory factory);
    bool unregisterApp(const char* appId);

    // Query
    DokiApp* createApp(const char* appId, uint8_t displayId);
    std::vector<AppMetadata> listApps() const;
    AppMetadata getMetadata(const char* appId) const;

private:
    std::map<std::string, AppEntry> registry_;
};
```

**Usage**:
```cpp
// In setup()
void registerBuiltInApps() {
    AppRegistry* registry = AppRegistry::getInstance();

    registry->registerApp(
        {"clock", "Digital Clock", "1.0.0", "Doki Team",
         32*1024, {}, AppType::NATIVE},
        [](uint8_t displayId) { return new ClockApp(); }
    );

    registry->registerApp(
        {"image", "Image Preview", "1.0.0", "Doki Team",
         128*1024, {AppPermission::STORAGE}, AppType::NATIVE},
        [](uint8_t displayId) { return new ImagePreviewApp(displayId); }
    );
}
```

**Benefits**:
- ✅ No code changes to add apps
- ✅ Apps registered at runtime
- ✅ Queryable catalog for API
- ✅ Foundation for app store

---

### 3.2 Memory Quota System

**Current Problem**: Apps can allocate unlimited memory, risking crashes.

**Solution**: Per-app memory tracking and enforcement

```cpp
class MemoryQuotaManager {
public:
    // Quota Management
    bool setQuota(const char* appId, size_t maxBytes);
    size_t getUsage(const char* appId) const;

    // Allocation Tracking
    void* allocate(const char* appId, size_t size, AllocType type);
    void free(const char* appId, void* ptr);

    // Cleanup
    void freeAllForApp(const char* appId);

private:
    struct AppMemory {
        size_t quota;
        size_t used;
        std::vector<Allocation> allocations;
    };

    std::map<std::string, AppMemory> appMemory_;
};
```

**Integration with DokiApp**:
```cpp
class DokiApp {
protected:
    void* allocMemory(size_t size) {
        return MemoryQuotaManager::getInstance()->allocate(
            metadata_.appId, size, AllocType::PSRAM
        );
    }

    void freeMemory(void* ptr) {
        MemoryQuotaManager::getInstance()->free(metadata_.appId, ptr);
    }
};
```

**Benefits**:
- ✅ Prevents crashes from leaks
- ✅ Apps can't starve each other
- ✅ Automatic cleanup on unload

---

### 3.3 Sandboxing for Native Apps

**Current Problem**: Native apps have full system access.

**Solution**: Permission system with resource isolation

```cpp
enum class AppPermission {
    NONE           = 0,
    DISPLAY        = 1 << 0,
    NETWORK        = 1 << 1,
    STORAGE        = 1 << 2,
    STORAGE_WRITE  = 1 << 3,
    SYSTEM_INFO    = 1 << 4
};

class SandboxManager {
public:
    bool hasPermission(const char* appId, AppPermission perm) const;
    void setPermissions(const char* appId, AppPermissions perms);

    // File system isolation
    String getAppDirectory(const char* appId) const;  // /apps/<appId>/
    bool canAccessFile(const char* appId, const char* path) const;

private:
    struct AppSandbox {
        AppPermissions permissions;
        String dataDirectory;
    };

    std::map<std::string, AppSandbox> sandboxes_;
};
```

**File System Isolation**:
```cpp
class SandboxedStorageManager {
public:
    bool writeFile(const char* appId, const char* relativePath,
                  const uint8_t* data, size_t len) {
        // Check permission
        if (!SandboxManager::hasPermission(appId, AppPermission::STORAGE_WRITE))
            return false;

        // Scope to app directory
        String fullPath = "/apps/" + String(appId) + "/" + String(relativePath);

        // Prevent directory traversal
        if (fullPath.indexOf("..") >= 0) return false;

        return FilesystemManager::writeFile(fullPath, data, len);
    }
};
```

**Benefits**:
- ✅ Apps can't corrupt other apps' data
- ✅ Network access controlled
- ✅ Foundation for app store trust

---

### 3.4 App State Persistence

**Current Problem**: App state lost when switching - poor UX.

**Solution**: Automatic state serialization using NVS

```cpp
class StatePersistence {
public:
    static bool init();

    // Save/Load
    static bool saveString(const char* appId, const char* key, const char* value);
    static String loadString(const char* appId, const char* key, const String& defaultValue = "");
    static bool saveInt(const char* appId, const char* key, int32_t value);
    static int32_t loadInt(const char* appId, const char* key, int32_t defaultValue = 0);

    // Cleanup
    static void clearState(const char* appId);

private:
    static nvs_handle_t _nvsHandle;
    static String _makeKey(const char* appId, const char* key);
};
```

**DokiApp Integration**:
```cpp
class DokiApp {
public:
    virtual void onSaveState() {}       // Called before pause/destroy
    virtual void onRestoreState() {}    // Called after onCreate

protected:
    void saveState(const char* key, const char* value);
    String loadState(const char* key, const String& defaultValue = "");
};

// Example: WeatherApp
class WeatherApp : public DokiApp {
    void onSaveState() override {
        saveState("temperature", String(currentTemp).c_str());
        saveState("location", currentLocation.c_str());
    }

    void onRestoreState() override {
        currentTemp = loadState("temperature", "0").toFloat();
        currentLocation = loadState("location", "Unknown");
        updateDisplay();  // Show cached data immediately
        fetchWeatherData();  // Fetch fresh in background
    }
};
```

**Benefits**:
- ✅ Instant app switching (cached data)
- ✅ No data loss
- ✅ Reduces API calls

---

### 3.5 Event System for App Communication

**Current Problem**: Apps are isolated, can't communicate.

**Solution**: Pub/Sub event system (already implemented in EventSystem!)

```cpp
enum class EventType {
    // System Events
    WIFI_CONNECTED,
    WIFI_DISCONNECTED,
    TIME_CHANGED,
    LOW_MEMORY,

    // App Events
    APP_DATA_UPDATED,
    USER_ACTION
};

class EventSystem {
public:
    uint32_t subscribe(EventType type, EventCallback callback);
    void publish(EventType type, const char* source, void* data = nullptr);
    void processEvents();  // Call from main loop
};
```

**DokiApp Integration**:
```cpp
class DokiApp {
protected:
    uint32_t subscribe(EventType type, EventCallback callback) {
        return EventSystem::getInstance()->subscribe(type, metadata_.appId, callback);
    }

    void publish(EventType type, void* data = nullptr) {
        EventSystem::getInstance()->publish(type, metadata_.appId, data);
    }
};

// Example: Weather app publishes temperature
class WeatherApp : public DokiApp {
    void updateTemperature(float temp) {
        currentTemp = temp;
        publish(EventType::APP_DATA_UPDATED, &temp, sizeof(float));
    }
};
```

**Benefits**:
- ✅ Apps react to system events
- ✅ Decoupled communication
- ✅ Async event delivery

---

### 3.6 Decoupling Logic from main.cpp

**Current Problem**: 600+ lines in main.cpp, hard to maintain.

**Solution**: Extract to dedicated managers

**Target File Structure**:
```
src/doki/
├── core/
│   ├── app_manager.cpp          # Existing (update for multi-display)
│   ├── display_manager.cpp      # Existing (integrate)
│   ├── event_system.cpp         # Existing (activate)
│   ├── memory_manager.cpp       # Existing (activate)
│   ├── task_scheduler.cpp       # Existing
│   ├── state_persistence.cpp    # NEW
│   ├── sandbox_manager.cpp      # NEW
│   └── memory_quota_manager.cpp # NEW
├── services/
│   ├── simple_http_server.cpp   # Existing
│   ├── filesystem_manager.cpp   # Existing
│   ├── storage_manager.cpp      # Existing (NVS)
│   ├── media_service.cpp        # Existing
│   ├── weather_service.cpp      # Existing
│   └── api_client.cpp           # Existing
├── drivers/
│   ├── lvgl_fs_driver.cpp       # Existing
│   ├── lvgl_mem.cpp             # New (PSRAM allocator)
│   └── st7789_driver.cpp        # Existing
└── utils/
    ├── qr_generator.cpp         # Existing
    ├── time_manager.cpp         # Existing
    └── setup_portal.cpp         # Existing
```

**Target main.cpp** (~150 lines):
```cpp
#include "doki/core/app_manager.h"
#include "doki/core/event_system.h"
#include "doki/core/memory_manager.h"
#include "doki/core/state_persistence.h"

using namespace Doki;

void setup() {
    Serial.begin(115200);

    // Initialize managers
    EventSystem::init();
    MemoryManager::init();
    StatePersistence::init();
    FilesystemManager::init();
    MediaService::init();
    AppManager::init();
    DisplayManager::init(2);  // 2 displays

    // Register apps
    registerBuiltInApps();

    // WiFi & HTTP
    if (!connectToWiFi()) {
        enterSetupMode();
        return;
    }
    SimpleHttpServer::begin();

    // Load default apps
    AppManager::loadApp(0, "clock");
    AppManager::loadApp(1, "weather");
}

void loop() {
    AppManager::update();
    EventSystem::processEvents();
    lv_timer_handler();
    delay(5);
}
```

---

## 4. Refactoring Strategy

### 4.1 Files to DELETE

1. ❌ `include/doki/http_server.h` + `src/doki/http_server.cpp`
   - **Reason**: Duplicate of simple_http_server
   - **Verification**: main.cpp uses SimpleHttpServer ✓

2. ⚠️ `include/doki/wifi_manager.h` + `src/doki/wifi_manager.cpp`
   - **Action**: Grep for usage first
   - **If unused**: DELETE

### 4.2 Files to CREATE

1. **include/doki/state_persistence.h** + **src/doki/state_persistence.cpp**
   - App state save/restore using NVS

2. **include/doki/sandbox_manager.h** + **src/doki/sandbox_manager.cpp**
   - Permission system (basic for now)

3. **include/doki/memory_quota_manager.h** + **src/doki/memory_quota_manager.cpp**
   - Per-app memory limits

### 4.3 Files to UPDATE

1. **include/doki/app_manager.h** + **src/doki/app_manager.cpp**
   - Add multi-display support
   - Change: `static DokiApp* _currentApp;` → `static std::map<uint8_t, DokiApp*> _displayApps;`
   - Add: `loadApp(uint8_t displayId, const char* appId)`

2. **include/doki/app_base.h** + **src/doki/app_base.cpp**
   - Add lifecycle hooks: `onSaveState()`, `onRestoreState()`, `onEvent()`
   - Add helpers: `saveState()`, `loadState()`, `subscribe()`, `publish()`

3. **src/main.cpp**
   - Remove manual display management
   - Replace createApp() with AppManager::loadApp()
   - Add EventSystem::processEvents()
   - Reduce from 600+ to ~200 lines

### 4.4 Implementation Order (4 Weeks)

**Week 1: Audit & Foundation**
- Day 1: Delete duplicates (http_server, check wifi_manager)
- Day 2-3: Update AppManager for multi-display
- Day 4-5: Create StatePersistence module

**Week 2: New Components**
- Day 1-2: Create SandboxManager (basic)
- Day 3-4: Create MemoryQuotaManager (basic)
- Day 5: Update DokiApp base class

**Week 3: Integration**
- Day 1-2: Refactor main.cpp to use managers
- Day 3-4: Update existing apps (state persistence)
- Day 5: Fix media upload issues (PSRAM, display selection)

**Week 4: Testing**
- Day 1-2: Test all apps with new architecture
- Day 3-4: Memory leak testing, stress testing
- Day 5: Documentation and cleanup

---

## 5. Component Design

### 5.1 Updated AppManager (Multi-Display Support)

```cpp
class AppManager {
public:
    // Registration
    static bool registerApp(const char* id, const char* name,
                           AppFactory factory, const char* description = "");

    // Multi-Display Lifecycle
    static bool loadApp(uint8_t displayId, const char* appId);
    static bool unloadApp(uint8_t displayId);
    static void update();  // Updates all active apps

    // Query
    static DokiApp* getCurrentApp(uint8_t displayId);
    static const char* getCurrentAppId(uint8_t displayId);
    static std::vector<AppRegistration> getRegisteredApps();

private:
    static std::map<std::string, AppRegistration> _registry;
    static std::map<uint8_t, DokiApp*> _displayApps;  // NEW: Per-display apps
    static std::map<uint8_t, std::string> _displayAppIds;
};
```

### 5.2 Updated DokiApp Base Class

```cpp
class DokiApp {
public:
    virtual ~DokiApp() {}

    // Core Lifecycle (must implement)
    virtual void onCreate() = 0;
    virtual void onStart() = 0;
    virtual void onUpdate() {}
    virtual void onPause() {}
    virtual void onDestroy() = 0;

    // NEW: State Management
    virtual void onSaveState() {}
    virtual void onRestoreState() {}

    // NEW: Event Handling
    virtual void onEvent(const Event& event) {}

    // System Access
    lv_obj_t* getScreen();
    void log(const char* message);

protected:
    // NEW: State Helpers
    void saveState(const char* key, const char* value);
    String loadState(const char* key, const String& defaultValue = "");

    // NEW: Event Helpers
    uint32_t subscribe(EventType type, EventCallback callback);
    void publish(EventType type, void* data = nullptr, size_t dataSize = 0);

    // NEW: Memory Helpers
    void* allocMemory(size_t size, AllocType type = AllocType::PSRAM);
    void freeMemory(void* ptr);
    size_t getMemoryUsage() const;

    // NEW: Permission Helpers
    bool hasPermission(AppPermission perm) const;

    AppMetadata metadata_;
    lv_obj_t* screen_;
};
```

### 5.3 App Metadata Structure

```cpp
struct AppMetadata {
    const char* appId;           // Unique ID
    const char* name;            // Display name
    const char* version;         // Version string
    const char* author;          // Author name
    const char* description;     // Short description
    size_t memoryRequired;       // Memory quota (bytes)
    AppPermissions permissions;  // Permission flags
    AppType type;                // NATIVE or JAVASCRIPT
};

enum class AppType {
    NATIVE,       // Compiled C++ app
    JAVASCRIPT    // Interpreted JS app (future)
};
```

---

## 6. Mobile Application

### 6.1 Technology Stack

- **Framework**: React Native (iOS + Android)
- **UI**: React Native Paper (Material Design)
- **HTTP**: Axios
- **Discovery**: React Native Zeroconf (mDNS)
- **Editor**: CodeMirror/Monaco

### 6.2 Features Roadmap

**Phase 1 (Weeks 10-11)**: MVP
- Device discovery via mDNS
- Device connection & control
- View/switch apps
- System status monitoring

**Phase 2 (Week 12)**: App Store
- Browse bundled apps
- Install/uninstall apps
- App details & screenshots

**Phase 3 (Week 13)**: Code Editor
- Write JavaScript apps
- Syntax highlighting
- Upload custom apps
- Test on device

---

## 7. Development Phases & Timeline

### Phase 0: Current State → Refactored (Weeks 1-4)

**Goal**: Clean up architecture, activate existing managers

**Tasks**:
- [ ] Delete duplicate files
- [ ] Update AppManager for multi-display
- [ ] Create StatePersistence, SandboxManager, MemoryQuotaManager
- [ ] Update DokiApp base class
- [ ] Refactor main.cpp
- [ ] Fix media upload issues

**Success Criteria**:
- ✅ main.cpp < 200 lines
- ✅ All managers active
- ✅ Memory leak detection working
- ✅ Event system propagating
- ✅ State persists between switches

---

### Phase 1: Foundation Complete (Weeks 5-6)

**Goal**: Solidify core OS with all built-in apps

**Tasks**:
- [ ] Polish all 8 existing apps
- [ ] Add state persistence to Weather, SysInfo
- [ ] Stress test (1000+ load/unload cycles)
- [ ] 24-hour stability test

**Success Criteria**:
- ✅ Zero crashes in stress tests
- ✅ Zero memory leaks
- ✅ 24+ hour stability
- ✅ Fast app switching (<500ms)

---

### Phase 2: JavaScript Runtime (Weeks 7-8)

**Goal**: Dynamic app loading with JavaScript

**Tasks**:
- [ ] Integrate Duktape (~50KB)
- [ ] Create JavaScript bindings (LVGL, Network, Storage)
- [ ] Implement sandbox
- [ ] Add /api/apps/upload endpoint
- [ ] Test with example JS apps

**Success Criteria**:
- ✅ Can upload .js app via API
- ✅ JS app runs correctly
- ✅ Bindings work (createLabel, httpGet, etc.)
- ✅ Sandboxing enforced (memory limits, timeouts)

---

### Phase 3: Multi-Display Enhancement (Week 9)

**Goal**: Smooth transitions, better display management

**Tasks**:
- [ ] Add transition effects (fade, slide)
- [ ] Support 3 displays (architecture ready)
- [ ] App-to-display assignment UI

**Success Criteria**:
- ✅ Smooth transitions
- ✅ Works with 1-3 displays
- ✅ Easy to add more

---

### Phase 4: Mobile App MVP (Weeks 10-11)

**Goal**: Device control from mobile

**Tasks**:
- [ ] React Native project setup
- [ ] Device discovery (mDNS)
- [ ] Device connection
- [ ] App list & switching UI
- [ ] System status dashboard

**Success Criteria**:
- ✅ Discovers devices < 10s
- ✅ Can switch apps from mobile
- ✅ Works on iOS & Android

---

### Phase 5: Mobile App Store (Week 12)

**Goal**: App marketplace

**Tasks**:
- [ ] App store UI
- [ ] Bundle 5-10 apps
- [ ] App installation flow
- [ ] App details screen

**Success Criteria**:
- ✅ Browse apps
- ✅ Install apps < 10s
- ✅ Apps work on device

---

### Phase 6: Mobile Code Editor (Week 13)

**Goal**: Create apps in mobile app

**Tasks**:
- [ ] Code editor component
- [ ] Syntax highlighting
- [ ] App templates
- [ ] Test on device

**Success Criteria**:
- ✅ Write JavaScript in app
- ✅ Upload custom apps
- ✅ Apps run correctly

---

### Phase 7: Testing & Polish (Week 14)

**Goal**: Production-ready

**Tasks**:
- [ ] Automated test suite
- [ ] Stress testing
- [ ] Performance optimization
- [ ] Complete documentation

**Success Criteria**:
- ✅ <300ms API response
- ✅ <500ms app load
- ✅ Complete docs

---

## 8. API Specification

### 8.1 System Endpoints

```
GET /api/system/info
GET /api/system/status
POST /api/system/restart
```

### 8.2 App Endpoints

```
GET    /api/apps/list
GET    /api/apps/:id
POST   /api/apps/upload
DELETE /api/apps/:id
POST   /api/apps/:id/switch?display=0
```

### 8.3 Display Endpoints

```
GET  /api/display/status
POST /api/display/brightness
POST /api/display/message
POST /api/display/:id/load?app=clock
```

### 8.4 Media Endpoints (Current)

```
GET    /api/media/info?display=0
POST   /api/media/upload
DELETE /api/media/delete?display=0&type=image
```

---

## 9. Security & Safety

### 9.1 Permission System

```cpp
enum class AppPermission {
    NONE           = 0,
    DISPLAY        = 1 << 0,  // UI creation
    NETWORK        = 1 << 1,  // HTTP requests
    STORAGE        = 1 << 2,  // Read files
    STORAGE_WRITE  = 1 << 3,  // Write files
    SYSTEM_INFO    = 1 << 4   // Read system stats
};
```

### 9.2 Memory Limits

- Default: 128KB per app (configurable)
- PSRAM allocation for large buffers
- Automatic cleanup on unload
- Quota enforcement via MemoryQuotaManager

### 9.3 File System Isolation

- Each app gets `/apps/<appId>/` directory
- No access to other apps' directories
- No `..` traversal allowed
- System files read-only

---

## 10. Testing Strategy

### 10.1 Unit Tests

```cpp
TEST(AppManager, MultiDisplay) {
    AppManager::loadApp(0, "clock");
    AppManager::loadApp(1, "weather");
    ASSERT_EQ(AppManager::getCurrentAppId(0), "clock");
    ASSERT_EQ(AppManager::getCurrentAppId(1), "weather");
}

TEST(MemoryQuotaManager, QuotaEnforcement) {
    MemoryQuotaManager::setQuota("test", 1024);
    void* ptr = MemoryQuotaManager::allocate("test", 2048);
    ASSERT_EQ(ptr, nullptr);  // Should fail (exceeds quota)
}

TEST(StatePersistence, SaveLoad) {
    StatePersistence::saveString("weather", "temp", "25");
    String temp = StatePersistence::loadString("weather", "temp");
    ASSERT_EQ(temp, "25");
}
```

### 10.2 Stress Tests

- 1000+ app load/unload cycles
- 24-hour continuous operation
- Rapid app switching (every 5s)
- Memory leak detection

### 10.3 Performance Tests

- App load time < 500ms
- App switch time < 200ms
- API response < 300ms
- Display FPS > 30

---

## 11. Success Metrics

### 11.1 Code Quality

- ✅ main.cpp < 200 lines
- ✅ All logic in dedicated managers
- ✅ No duplicate code
- ✅ Clear module boundaries

### 11.2 Functionality

- ✅ All existing features working
- ✅ State persistence active
- ✅ Memory leak detection active
- ✅ Event-based communication

### 11.3 Performance

- ✅ No performance degradation
- ✅ Memory usage tracked
- ✅ Fast app switching (<500ms)
- ✅ 24+ hour stability

### 11.4 Maintainability

- ✅ Easy to add new apps
- ✅ Easy to add new features
- ✅ Well-documented APIs
- ✅ Modular architecture

---

## 12. Risk Assessment

### 12.1 Low Risk ✅

- Using existing AppManager, EventSystem, MemoryManager
- State persistence (simple NVS operations)
- Updating DokiApp (backward compatible)

### 12.2 Medium Risk ⚠️

- Multi-display support in AppManager
- Refactoring main.cpp
- Memory quota enforcement

### 12.3 Mitigation

- Git branch before starting
- Test each phase before proceeding
- Keep existing apps working
- Can rollback if issues

---

## 13. Next Steps

### Immediate Actions

1. ✅ Review this combined plan
2. ✅ Create git branch: `refactor/modular-architecture`
3. ✅ Verify wifi_manager usage: `grep -r "WifiManager" src/ include/`
4. ✅ Delete http_server.h/cpp
5. ✅ Start Week 1: Update AppManager for multi-display

### Questions to Answer

1. Delete wifi_manager.h? (Check usage first)
2. Keep current media upload PSRAM fix?
3. Start refactoring immediately or test media upload first?

---

## Appendix A: Flash Memory Layout

```
Flash Memory (16MB):
├── Bootloader           (64KB)
├── Partition Table      (12KB)
├── OTA_0 (Firmware)     (3MB)
├── OTA_1 (Backup)       (3MB)
├── NVS (Settings)       (20KB)
├── SPIFFS (Files)       (10MB)
│   ├── /system/
│   │   └── config.json
│   ├── /apps/
│   │   ├── <appId>/
│   │   │   └── data/
│   │   └── manifest.json
│   ├── /media/
│   │   ├── d0_image.png
│   │   ├── d0_anim.gif
│   │   ├── d1_image.png
│   │   └── d1_anim.gif
│   └── /cache/
└── Reserved             (512KB)
```

---

## Appendix B: Current vs Target Comparison

| Aspect | Current | Target |
|--------|---------|--------|
| **main.cpp lines** | 600+ | ~200 |
| **App registry** | Hardcoded | Dynamic |
| **Memory tracking** | None | Per-app quotas |
| **State persistence** | None | NVS-based |
| **App communication** | None | Event system |
| **Permissions** | Full access | Sandboxed |
| **Multi-display** | Manual | AppManager |
| **Memory leaks** | Unknown | Detected |

---

## Conclusion

This plan combines our **excellent existing infrastructure** with **critical architectural improvements** to create a production-ready, extensible OS.

**Key Advantages**:
1. We already have 50%+ of the code written (AppManager, EventSystem, MemoryManager)
2. Clear 4-week refactoring path
3. Backward compatible - existing apps keep working
4. Low risk - can rollback if needed
5. Modular approach - easy to test each phase

**Timeline**:
- **Weeks 1-4**: Refactoring (activate existing, create missing, clean up)
- **Weeks 5-6**: Solidify foundation
- **Weeks 7-8**: JavaScript runtime
- **Weeks 9-14**: Mobile app + polish

**Ready to build Doki OS!** 🚀
