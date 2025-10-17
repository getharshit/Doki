# JavaScript Apps for Doki OS

Doki OS supports running JavaScript applications using the **Duktape** JavaScript engine. This allows you to write apps in JavaScript instead of C++.

## 🚀 Quick Start

### 1. Enable JavaScript Support

JavaScript support is **disabled by default** to save flash space. To enable it:

#### Step 1: Download Duktape

```bash
cd /Users/harshit/Desktop/Doki
wget https://duktape.org/duktape-2.7.0.tar.xz
tar -xf duktape-2.7.0.tar.xz
```

#### Step 2: Copy Duktape Files

```bash
cp duktape-2.7.0/src/duktape.c lib/duktape/
cp duktape-2.7.0/src/duktape.h lib/duktape/
```

#### Step 3: Enable in Code

Edit `include/doki/js_engine.h` and uncomment:

```cpp
#define ENABLE_JAVASCRIPT_SUPPORT
```

#### Step 4: Rebuild

```bash
pio run
```

### 2. Create Your First JS App

Create `/data/apps/myapp.js`:

```javascript
function onCreate() {
    log("My first JS app!");
    setBackgroundColor(0x001122);
    createLabel("Hello Doki OS!", 50, 140);
}

function onUpdate() {
    // Update logic here
}

function onDestroy() {
    log("Goodbye!");
}
```

### 3. Register and Load the App

In `main.cpp`, add:

```cpp
#include "doki/js_app.h"

// In setup(), after other app registrations:
Doki::AppManager::registerApp("myapp", "My JS App",
    []() -> Doki::DokiApp* {
        return new Doki::JSApp("myapp", "My JS App", "/apps/myapp.js");
    },
    "My first JavaScript app");
```

### 4. Upload JS Files to SPIFFS

```bash
pio run --target uploadfs
```

### 5. Load via HTTP API

```bash
curl -X POST "http://192.168.1.xxx/api/load?display=0&app=myapp"
```

## 📚 JavaScript API Reference

### Logging

```javascript
log(message)
```

Print message to serial console.

**Example:**
```javascript
log("App started!");
log("Counter: " + count);
```

### LVGL UI Functions

#### createLabel(text, x, y)

Create a text label at position (x, y).

**Example:**
```javascript
createLabel("Hello World", 60, 100);
createLabel("Score: " + score, 80, 150);
```

#### createButton(text, x, y)

Create a button with text at position (x, y).

**Example:**
```javascript
createButton("Click Me", 60, 200);
createButton("Start", 80, 180);
```

#### setBackgroundColor(hexColor)

Set screen background color using hex code.

**Example:**
```javascript
setBackgroundColor(0x001122);  // Dark blue
setBackgroundColor(0xFF0000);  // Red
setBackgroundColor(0x000000);  // Black
```

### Display Information

#### getDisplayId()

Get the ID of the display this app is running on (0, 1, or 2).

**Example:**
```javascript
var displayId = getDisplayId();
log("Running on display: " + displayId);
```

### State Persistence

#### saveState(key, value)

Save a key-value pair to persistent storage (NVS).

**Example:**
```javascript
saveState("highScore", "1000");
saveState("userName", "Alice");
```

#### loadState(key)

Load a value from persistent storage.

**Returns:** String value or null if not found

**Example:**
```javascript
var highScore = loadState("highScore");
if (highScore) {
    log("High score: " + highScore);
} else {
    log("No saved high score");
}
```

## 🔄 App Lifecycle

JavaScript apps follow the same lifecycle as C++ apps:

```javascript
function onCreate() {
    // Called once when app is created
    // Initialize UI, load resources
}

function onStart() {
    // Called when app becomes visible
    // Start animations, timers
}

function onUpdate() {
    // Called every ~100ms while app is running
    // Keep this lightweight!
    // Update UI, handle logic
}

function onPause() {
    // Called when app goes to background
    // Stop animations, save state
}

function onDestroy() {
    // Called before app is unloaded
    // Cleanup, save final state
}

// Optional state persistence
function onSaveState() {
    saveState("myData", "value");
    return { timestamp: Date.now() };
}

function onRestoreState(state) {
    var data = loadState("myData");
    log("Restored: " + data);
}
```

## 📝 Example Apps

### Hello World

```javascript
function onCreate() {
    log("Hello World!");
    setBackgroundColor(0x001133);
    createLabel("Hello from", 60, 100);
    createLabel("JavaScript!", 50, 130);
    createButton("Click Me", 60, 200);
}

function onUpdate() {
    // Nothing to update
}

function onDestroy() {
    log("Goodbye!");
}
```

### Counter with Persistence

```javascript
var count = 0;

function onCreate() {
    setBackgroundColor(0x001a33);
    createLabel("Counter App", 60, 60);
    updateDisplay();
}

function onUpdate() {
    count++;
    updateDisplay();
}

function updateDisplay() {
    createLabel("Count: " + count, 80, 120);
}

function onSaveState() {
    saveState("count", count.toString());
}

function onRestoreState(state) {
    var saved = loadState("count");
    if (saved) {
        count = parseInt(saved);
    }
}

function onDestroy() {
    log("Final count: " + count);
}
```

### Display-Specific App

```javascript
function onCreate() {
    var displayId = getDisplayId();
    log("Running on display " + displayId);

    if (displayId === 0) {
        setBackgroundColor(0x110000);  // Red for display 0
        createLabel("Display 0", 70, 140);
    } else {
        setBackgroundColor(0x001100);  // Green for display 1
        createLabel("Display 1", 70, 140);
    }
}

function onUpdate() {}
function onDestroy() {}
```

## 🎯 Best Practices

### 1. Keep onUpdate() Lightweight

`onUpdate()` is called every ~100ms. Avoid heavy computations:

```javascript
// ❌ Bad - creates new objects every frame
function onUpdate() {
    var now = new Date();
    createLabel("Time: " + now, 60, 100);  // Recreates label!
}

// ✅ Good - update only when needed
var lastSecond = 0;
function onUpdate() {
    var now = Date.now() / 1000;
    if (Math.floor(now) !== lastSecond) {
        lastSecond = Math.floor(now);
        // Update display
    }
}
```

### 2. Use State Persistence

Save important data so it survives app reloads:

```javascript
function onSaveState() {
    saveState("level", level.toString());
    saveState("score", score.toString());
}

function onRestoreState(state) {
    level = parseInt(loadState("level") || "1");
    score = parseInt(loadState("score") || "0");
}
```

### 3. Log Errors for Debugging

```javascript
function onCreate() {
    try {
        // Your code here
        riskyOperation();
    } catch (e) {
        log("Error: " + e);
    }
}
```

### 4. Clean Up Resources

```javascript
var timerHandle = null;

function onCreate() {
    timerHandle = setInterval(function() {
        log("Tick");
    }, 1000);
}

function onDestroy() {
    if (timerHandle) {
        clearInterval(timerHandle);
        timerHandle = null;
    }
}
```

## 📦 File Structure

```
/data/apps/
├── hello.js          # Hello World example
├── counter.js        # Counter with persistence
├── clock.js          # Custom clock app
└── weather.js        # Weather display app
```

## 🔧 Advanced: Extending the API

To add more JavaScript APIs, edit `src/doki/js_engine.cpp`:

```cpp
// Add new function binding
duk_ret_t JSEngine::_js_myNewFunction(duk_context* ctx) {
    int arg = duk_to_int(ctx, 0);
    // Your C++ logic here
    duk_push_int(ctx, result);
    return 1;  // Number of return values
}

// Register in registerDokiAPIs()
duk_push_c_function(duk_ctx, _js_myNewFunction, 1);
duk_put_global_string(duk_ctx, "myNewFunction");
```

Then use in JavaScript:

```javascript
var result = myNewFunction(42);
```

## 🐛 Troubleshooting

### JavaScript support not enabled

**Error:** "JavaScript support not enabled"

**Solution:** Follow the "Enable JavaScript Support" steps above.

### Script not found

**Error:** "Failed to open: /apps/myapp.js"

**Solution:**
1. Ensure file exists in `/data/apps/`
2. Run `pio run --target uploadfs` to upload SPIFFS
3. Check file path is correct

### Function not defined

**Error:** "Function error: ReferenceError: myFunc is not defined"

**Solution:** Ensure function is defined before calling it:

```javascript
function myFunc() {
    // ...
}

myFunc();  // Call after definition
```

### Memory issues

**Symptom:** App crashes or behaves strangely

**Solution:**
- Keep `onUpdate()` lightweight
- Don't create too many LVGL objects
- Clear unused variables

## 📊 Performance Notes

- **Duktape overhead:** ~50KB flash + ~20KB RAM
- **JS execution:** ~10-50x slower than C++ (still fast enough for UI apps)
- **Recommended app size:** < 10KB JavaScript code
- **Update frequency:** Throttled to 100ms (~10 FPS)

## 🚀 Next Steps

1. **Try the examples** in `/data/apps/`
2. **Create your own app** - start simple!
3. **Explore LVGL** for more UI widgets
4. **Share your apps** - they're just JavaScript files!

## 📖 Resources

- **Duktape Docs:** https://duktape.org/guide.html
- **LVGL Docs:** https://docs.lvgl.io/8.3/
- **Doki OS Source:** Check `include/doki/js_engine.h` for all available APIs

---

**Happy JavaScript coding on Doki OS! 🎉**
