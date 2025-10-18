# Doki OS - JavaScript API Reference

Complete reference for writing JavaScript apps on Doki OS using the embedded Duktape engine.

---

## Table of Contents

1. [App Lifecycle](#app-lifecycle)
2. [UI Functions](#ui-functions)
3. [Animation System](#animation-system)
4. [Display Management](#display-management)
5. [Network Functions](#network-functions)
6. [State Persistence](#state-persistence)
7. [Utility Functions](#utility-functions)
8. [Complete Examples](#complete-examples)

---

## App Lifecycle

Every JavaScript app must implement these lifecycle functions:

### `onCreate()`

Called once when the app is loaded. Initialize your app here.

```javascript
function onCreate() {
    log("My App - Starting");
    setBackgroundColor(0x000000);  // Black background
    createLabel("Hello World", 120, 160);
}
```

### `onUpdate()`

Called every frame (~30 FPS). Update animation, check timers, etc.

```javascript
var frameCount = 0;

function onUpdate() {
    frameCount++;

    // Update animations
    updateAnimations();

    // Your update logic here
}
```

### `onPause()` *(Optional)*

Called when app is paused (e.g., switching to another display).

```javascript
function onPause() {
    log("App paused");
    // Pause animations, save state, etc.
}
```

### `onDestroy()` *(Optional)*

Called when app is unloaded. Clean up resources here.

```javascript
function onDestroy() {
    log("App cleanup");
    // Unload animations, close connections, etc.
}
```

---

## UI Functions

### Labels

#### `createLabel(text, x, y)`

Create a text label at position (x, y).

**Parameters:**
- `text` (string): Text to display
- `x` (number): X coordinate (0-240)
- `y` (number): Y coordinate (0-320)

**Returns:** Label ID (number)

**Example:**
```javascript
var titleLabel = createLabel("Temperature", 120, 50);
var valueLabel = createLabel("25°C", 120, 100);
```

#### `updateLabel(labelId, text)`

Update existing label text.

**Parameters:**
- `labelId` (number): Label ID returned from createLabel()
- `text` (string): New text

**Example:**
```javascript
var temp = 26;
updateLabel(valueLabel, temp + "°C");
```

#### `setLabelColor(labelId, color)`

Set label text color.

**Parameters:**
- `labelId` (number): Label ID
- `color` (number): RGB color in hex (0xRRGGBB)

**Example:**
```javascript
setLabelColor(titleLabel, 0xFF0000);  // Red
setLabelColor(valueLabel, 0x00FF00);  // Green
```

#### `setLabelSize(labelId, size)`

Set label font size.

**Parameters:**
- `labelId` (number): Label ID
- `size` (number): Font size (12, 16, 20, 24, etc.)

**Example:**
```javascript
setLabelSize(titleLabel, 24);  // Large title
setLabelSize(valueLabel, 16);  // Smaller value
```

#### `createScrollingLabel(text, x, y, width)`

Create a horizontally scrolling label (for long text).

**Parameters:**
- `text` (string): Text to scroll
- `x` (number): X coordinate
- `y` (number): Y coordinate
- `width` (number): Label width in pixels

**Returns:** Label ID (number)

**Example:**
```javascript
var scrollLabel = createScrollingLabel(
    "This is a very long text that will scroll horizontally",
    120, 200, 200
);
```

### Drawing Functions

#### `setBackgroundColor(color)`

Set screen background color.

**Parameters:**
- `color` (number): RGB color in hex (0xRRGGBB)

**Example:**
```javascript
setBackgroundColor(0x000000);  // Black
setBackgroundColor(0x1a1a2e);  // Dark blue
setBackgroundColor(0xFFFFFF);  // White
```

#### `clearScreen()`

Clear the screen (removes all UI elements).

**Example:**
```javascript
clearScreen();
```

#### `drawRectangle(x, y, width, height, color)`

Draw a filled rectangle.

**Parameters:**
- `x` (number): X coordinate
- `y` (number): Y coordinate
- `width` (number): Rectangle width
- `height` (number): Rectangle height
- `color` (number): RGB color (0xRRGGBB)

**Example:**
```javascript
drawRectangle(50, 50, 140, 100, 0xFF0000);  // Red rectangle
```

#### `drawCircle(centerX, centerY, radius, color)`

Draw a filled circle.

**Parameters:**
- `centerX` (number): Center X coordinate
- `centerY` (number): Center Y coordinate
- `radius` (number): Circle radius in pixels
- `color` (number): RGB color (0xRRGGBB)

**Example:**
```javascript
drawCircle(120, 160, 50, 0x00FF00);  // Green circle
```

### Buttons

#### `createButton(text, x, y, callback)`

Create a button (Note: Currently requires touch support).

**Parameters:**
- `text` (string): Button label
- `x` (number): X coordinate
- `y` (number): Y coordinate
- `callback` (string): JavaScript function name to call when pressed

**Example:**
```javascript
function onCreate() {
    createButton("Click Me", 120, 160, "onButtonClick");
}

function onButtonClick() {
    log("Button was clicked!");
}
```

### Animations & Effects

#### `fadeIn(labelId, duration)`

Fade in a label.

**Parameters:**
- `labelId` (number): Label ID
- `duration` (number): Duration in milliseconds

**Example:**
```javascript
var label = createLabel("Fading in...", 120, 160);
fadeIn(label, 1000);  // Fade in over 1 second
```

#### `fadeOut(labelId, duration)`

Fade out a label.

**Parameters:**
- `labelId` (number): Label ID
- `duration` (number): Duration in milliseconds

**Example:**
```javascript
fadeOut(label, 1000);  // Fade out over 1 second
```

#### `moveLabel(labelId, x, y, duration)`

Animate label movement to new position.

**Parameters:**
- `labelId` (number): Label ID
- `x` (number): Target X coordinate
- `y` (number): Target Y coordinate
- `duration` (number): Animation duration in milliseconds

**Example:**
```javascript
moveLabel(label, 200, 100, 500);  // Move to (200,100) in 0.5s
```

#### `setOpacity(labelId, opacity)`

Set label opacity.

**Parameters:**
- `labelId` (number): Label ID
- `opacity` (number): Opacity (0-255, where 255 = fully opaque)

**Example:**
```javascript
setOpacity(label, 128);  // 50% transparent
```

---

## Animation System

### Loading & Unloading

#### `loadAnimation(filepath)`

Load an animation sprite file.

**Parameters:**
- `filepath` (string): Path to .spr file (e.g., "/animations/spinner.spr")

**Returns:** Animation ID (number, >= 0) or -1 on error

**Example:**
```javascript
var spinnerAnim = loadAnimation("/animations/spinner.spr");
if (spinnerAnim < 0) {
    log("Failed to load animation");
}
```

#### `unloadAnimation(animationId)`

Unload animation and free memory.

**Parameters:**
- `animationId` (number): Animation ID

**Example:**
```javascript
unloadAnimation(spinnerAnim);
```

### Playback Control

#### `playAnimation(animationId, loop)`

Start playing an animation.

**Parameters:**
- `animationId` (number): Animation ID
- `loop` (boolean): true = loop forever, false = play once

**Returns:** `true` if started successfully, `false` on error

**Example:**
```javascript
playAnimation(spinnerAnim, true);   // Loop forever
playAnimation(checkAnim, false);    // Play once
```

#### `stopAnimation(animationId)`

Stop playing animation.

**Parameters:**
- `animationId` (number): Animation ID

**Example:**
```javascript
stopAnimation(spinnerAnim);
```

#### `pauseAnimation(animationId)`

Pause animation (can be resumed).

**Parameters:**
- `animationId` (number): Animation ID

**Example:**
```javascript
pauseAnimation(spinnerAnim);
```

#### `resumeAnimation(animationId)`

Resume paused animation.

**Parameters:**
- `animationId` (number): Animation ID

**Example:**
```javascript
resumeAnimation(spinnerAnim);
```

#### `updateAnimations()`

Update all playing animations. **Must call in onUpdate()**.

**Example:**
```javascript
function onUpdate() {
    updateAnimations();  // Required for animations to play
}
```

### Animation Properties

#### `setAnimationPosition(animationId, x, y)`

Set animation position on screen.

**Parameters:**
- `animationId` (number): Animation ID
- `x` (number): X coordinate (top-left corner)
- `y` (number): Y coordinate (top-left corner)

**Example:**
```javascript
// Center a 100×100 animation on 240×320 screen
setAnimationPosition(spinnerAnim, 70, 110);
```

#### `setAnimationSpeed(animationId, speed)`

Set animation playback speed.

**Parameters:**
- `animationId` (number): Animation ID
- `speed` (number): Speed multiplier (0.5 = half speed, 2.0 = double speed)

**Example:**
```javascript
setAnimationSpeed(spinnerAnim, 0.5);  // Slow motion
setAnimationSpeed(spinnerAnim, 2.0);  // Fast forward
```

#### `setAnimationOpacity(animationId, opacity)`

Set animation opacity.

**Parameters:**
- `animationId` (number): Animation ID
- `opacity` (number): Opacity (0-255)

**Example:**
```javascript
setAnimationOpacity(spinnerAnim, 128);  // 50% transparent
```

### Complete Animation Example

```javascript
var loadingAnim = -1;

function onCreate() {
    setBackgroundColor(0x000000);

    // Load and play loading spinner
    loadingAnim = loadAnimation("/animations/spinner.spr");

    if (loadingAnim >= 0) {
        setAnimationPosition(loadingAnim, 70, 110);  // Center
        playAnimation(loadingAnim, true);  // Loop
    }
}

function onUpdate() {
    updateAnimations();  // Required!
}

function onDestroy() {
    if (loadingAnim >= 0) {
        stopAnimation(loadingAnim);
        unloadAnimation(loadingAnim);
    }
}
```

---

## Display Management

### `getDisplayId()`

Get current display ID.

**Returns:** Display ID (0, 1, or 2)

**Example:**
```javascript
var displayId = getDisplayId();
log("Running on display " + displayId);
```

### `getDisplayCount()`

Get total number of displays.

**Returns:** Number of displays (1-3)

**Example:**
```javascript
var count = getDisplayCount();
log("System has " + count + " displays");
```

### `getWidth()`

Get display width in pixels.

**Returns:** Width (typically 240)

**Example:**
```javascript
var w = getWidth();
var centerX = w / 2;
```

### `getHeight()`

Get display height in pixels.

**Returns:** Height (typically 320)

**Example:**
```javascript
var h = getHeight();
var centerY = h / 2;
```

### `sendToDisplay(displayId, message)`

Send message to app on another display.

**Parameters:**
- `displayId` (number): Target display ID (0-2)
- `message` (string): Message to send

**Example:**
```javascript
// Send temperature reading to display 1
sendToDisplay(1, "temp:25.5");
```

---

## Network Functions

### HTTP

#### `httpGet(url, callback)`

Perform HTTP GET request.

**Parameters:**
- `url` (string): Full URL to fetch
- `callback` (string): JavaScript function name to call with response

**Example:**
```javascript
function onCreate() {
    httpGet("https://api.example.com/data", "onDataReceived");
}

function onDataReceived(response) {
    log("Response: " + response);
    // Parse and use response
}
```

### WebSocket

#### `wsConnect(url)`

Connect to WebSocket server.

**Parameters:**
- `url` (string): WebSocket URL (ws:// or wss://)

**Returns:** `true` if connection initiated

**Example:**
```javascript
wsConnect("ws://192.168.1.100:8080");
```

#### `wsIsConnected()`

Check if WebSocket is connected.

**Returns:** `true` if connected, `false` otherwise

**Example:**
```javascript
if (wsIsConnected()) {
    log("WebSocket is connected");
}
```

#### `wsSend(message)`

Send message over WebSocket.

**Parameters:**
- `message` (string): Message to send

**Example:**
```javascript
wsSend("Hello from Doki OS!");
```

#### `wsOnMessage(callback)`

Set callback for incoming WebSocket messages.

**Parameters:**
- `callback` (string): JavaScript function name

**Example:**
```javascript
function onCreate() {
    wsConnect("ws://192.168.1.100:8080");
    wsOnMessage("onWsMessage");
}

function onWsMessage(message) {
    log("Received: " + message);
}
```

#### `wsDisconnect()`

Disconnect from WebSocket.

**Example:**
```javascript
wsDisconnect();
```

### MQTT

#### `mqttConnect(broker, port, clientId)`

Connect to MQTT broker.

**Parameters:**
- `broker` (string): Broker hostname/IP
- `port` (number): Port (usually 1883)
- `clientId` (string): Unique client ID

**Example:**
```javascript
mqttConnect("broker.hivemq.com", 1883, "doki-device-001");
```

#### `mqttPublish(topic, message)`

Publish MQTT message.

**Parameters:**
- `topic` (string): MQTT topic
- `message` (string): Message payload

**Example:**
```javascript
mqttPublish("sensors/temperature", "25.5");
```

#### `mqttSubscribe(topic, callback)`

Subscribe to MQTT topic.

**Parameters:**
- `topic` (string): MQTT topic (wildcards supported)
- `callback` (string): JavaScript function name

**Example:**
```javascript
function onCreate() {
    mqttConnect("broker.hivemq.com", 1883, "doki-001");
    mqttSubscribe("commands/#", "onMqttMessage");
}

function onMqttMessage(topic, message) {
    log("Topic: " + topic + ", Message: " + message);
}
```

#### `mqttDisconnect()`

Disconnect from MQTT broker.

**Example:**
```javascript
mqttDisconnect();
```

---

## State Persistence

### `saveState(key, value)`

Save persistent state (survives app reload).

**Parameters:**
- `key` (string): State key
- `value` (string): Value to save

**Example:**
```javascript
var counter = 42;
saveState("counter", counter.toString());
```

### `loadState(key)`

Load persistent state.

**Parameters:**
- `key` (string): State key

**Returns:** Saved value (string) or `null` if not found

**Example:**
```javascript
function onCreate() {
    var saved = loadState("counter");
    if (saved !== null) {
        counter = parseInt(saved);
        log("Loaded counter: " + counter);
    }
}
```

---

## Utility Functions

### `log(message)`

Print message to Serial Monitor.

**Parameters:**
- `message` (string): Message to log

**Example:**
```javascript
log("Temperature sensor initialized");
log("Current value: " + temperature);
```

### `millis()`

Get milliseconds since boot.

**Returns:** Milliseconds (number)

**Example:**
```javascript
var startTime = millis();

function onUpdate() {
    var elapsed = millis() - startTime;
    if (elapsed > 5000) {
        log("5 seconds passed!");
    }
}
```

### `getTime()`

Get current Unix timestamp (seconds since 1970).

**Returns:** Unix timestamp (number)

**Requires:** NTP time sync

**Example:**
```javascript
var timestamp = getTime();
log("Current time: " + timestamp);
```

---

## Complete Examples

### Example 1: Clock App

```javascript
var timeLabel = -1;
var lastUpdate = 0;

function onCreate() {
    log("Clock App - Starting");
    setBackgroundColor(0x000000);
    timeLabel = createLabel("--:--:--", 120, 160);
    setLabelSize(timeLabel, 24);
    setLabelColor(timeLabel, 0x00FF00);
}

function onUpdate() {
    // Update time every second
    var now = millis();
    if (now - lastUpdate >= 1000) {
        lastUpdate = now;

        var timestamp = getTime();
        var date = new Date(timestamp * 1000);

        var hours = date.getHours();
        var minutes = date.getMinutes();
        var seconds = date.getSeconds();

        var timeStr =
            (hours < 10 ? "0" : "") + hours + ":" +
            (minutes < 10 ? "0" : "") + minutes + ":" +
            (seconds < 10 ? "0" : "") + seconds;

        updateLabel(timeLabel, timeStr);
    }
}
```

### Example 2: Temperature Sensor with Animation

```javascript
var tempLabel = -1;
var loadingAnim = -1;
var isLoading = true;

function onCreate() {
    setBackgroundColor(0x1a1a2e);

    createLabel("Temperature", 120, 80);
    tempLabel = createLabel("Loading...", 120, 140);
    setLabelSize(tempLabel, 20);

    // Show loading animation
    loadingAnim = loadAnimation("/animations/spinner.spr");
    if (loadingAnim >= 0) {
        setAnimationPosition(loadingAnim, 70, 180);
        playAnimation(loadingAnim, true);
    }

    // Fetch temperature
    httpGet("https://api.example.com/temperature", "onTempReceived");
}

function onUpdate() {
    updateAnimations();
}

function onTempReceived(response) {
    log("Temperature data: " + response);

    // Hide loading animation
    if (loadingAnim >= 0) {
        stopAnimation(loadingAnim);
        unloadAnimation(loadingAnim);
        loadingAnim = -1;
    }

    // Parse and display temperature
    var temp = parseFloat(response);
    updateLabel(tempLabel, temp.toFixed(1) + "°C");

    // Change color based on temperature
    if (temp > 30) {
        setLabelColor(tempLabel, 0xFF0000);  // Hot = Red
    } else if (temp < 15) {
        setLabelColor(tempLabel, 0x0000FF);  // Cold = Blue
    } else {
        setLabelColor(tempLabel, 0x00FF00);  // Normal = Green
    }
}

function onDestroy() {
    if (loadingAnim >= 0) {
        stopAnimation(loadingAnim);
        unloadAnimation(loadingAnim);
    }
}
```

### Example 3: WebSocket Live Dashboard

```javascript
var statusLabel = -1;
var valueLabel = -1;
var wsConnected = false;

function onCreate() {
    log("WebSocket Dashboard - Starting");
    setBackgroundColor(0x000000);

    statusLabel = createLabel("Connecting...", 120, 50);
    valueLabel = createLabel("--", 120, 160);
    setLabelSize(valueLabel, 24);

    // Connect to WebSocket server
    wsConnect("ws://192.168.1.100:8080");
    wsOnMessage("onWsMessage");
}

var lastCheck = 0;

function onUpdate() {
    var now = millis();

    // Check connection status every second
    if (now - lastCheck >= 1000) {
        lastCheck = now;

        var connected = wsIsConnected();
        if (connected && !wsConnected) {
            wsConnected = true;
            updateLabel(statusLabel, "Connected");
            setLabelColor(statusLabel, 0x00FF00);
        } else if (!connected && wsConnected) {
            wsConnected = false;
            updateLabel(statusLabel, "Disconnected");
            setLabelColor(statusLabel, 0xFF0000);
        }
    }
}

function onWsMessage(message) {
    log("WebSocket: " + message);
    updateLabel(valueLabel, message);
}

function onDestroy() {
    wsDisconnect();
}
```

---

## Screen Dimensions

- **Width**: 240 pixels
- **Height**: 320 pixels
- **Center**: (120, 160)

### Centering Formula

```javascript
// For a label/animation of width W and height H:
var x = (240 - W) / 2;
var y = (320 - H) / 2;
```

### Common Positions

```javascript
// Top center
createLabel("Title", 120, 20);

// Center
createLabel("Message", 120, 160);

// Bottom center
createLabel("Status", 120, 300);

// Top-left
createLabel("Info", 10, 10);

// Top-right
createLabel("Time", 230, 10);
```

---

## Best Practices

### 1. Always Call updateAnimations()

```javascript
function onUpdate() {
    updateAnimations();  // Required for animations to play
    // Your code here
}
```

### 2. Clean Up Resources

```javascript
function onDestroy() {
    // Unload animations
    if (myAnim >= 0) {
        stopAnimation(myAnim);
        unloadAnimation(myAnim);
    }

    // Disconnect network
    wsDisconnect();
    mqttDisconnect();
}
```

### 3. Check Return Values

```javascript
var anim = loadAnimation("/animations/spinner.spr");
if (anim < 0) {
    log("ERROR: Failed to load animation");
    return;
}
```

### 4. Use Throttling for Updates

```javascript
var lastUpdate = 0;

function onUpdate() {
    var now = millis();

    // Only update every 100ms
    if (now - lastUpdate >= 100) {
        lastUpdate = now;
        // Do expensive operations here
    }
}
```

### 5. Optimize Label Updates

```javascript
// BAD: Updates every frame
function onUpdate() {
    updateLabel(label, "Frame: " + frameCount);
}

// GOOD: Updates every second
function onUpdate() {
    if (frameCount % 30 === 0) {  // ~30 FPS
        updateLabel(label, "Frame: " + frameCount);
    }
}
```

---

## Memory Limits

- **JavaScript Heap**: 128 KB per app
- **Code Size**: 16 KB maximum
- **Animation Pool**: 1024 KB total (shared across all apps)

---

## Error Handling

### Common Errors

**"Out of memory"**
- Too many animations loaded
- JavaScript code too large
- Too many objects created

**"Animation load failed"**
- File not found
- Invalid .spr format
- Animation pool full

**"WebSocket connection failed"**
- Invalid URL
- Server not reachable
- Network not connected

### Debugging

```javascript
// Enable verbose logging
function onCreate() {
    log("=== App Starting ===");
    log("Display ID: " + getDisplayId());
    log("Display size: " + getWidth() + "x" + getHeight());
}

// Check file existence
var anim = loadAnimation("/animations/test.spr");
if (anim < 0) {
    log("ERROR: Failed to load animation");
} else {
    log("SUCCESS: Animation loaded, ID=" + anim);
}
```

---

## File Paths

All files are relative to SPIFFS root:

```
/animations/spinner.spr      ✓ Correct
/apps/myapp.js               ✓ Correct
animations/spinner.spr       ✗ Wrong (missing leading /)
/nonexistent/file.spr        ✗ Wrong (file doesn't exist)
```

---

## Support

For more examples, see:
- `data/apps/animation_chain.js` - Animation chaining
- `data/apps/stress_test.js` - System stress testing
- `data/apps/websocket_test.js` - WebSocket usage

For HTTP API documentation, see: [HTTP_REST_API.md](HTTP_REST_API.md)

For animation sprite creation, see: [ANIMATION_UPLOAD_API.md](ANIMATION_UPLOAD_API.md)
