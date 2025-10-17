/**
 * Advanced Demo JavaScript App for Doki OS - V3 (NON-BLOCKING)
 *
 * BEST PRACTICES:
 * - Non-blocking initialization (features load gradually)
 * - Graceful degradation (works even if services fail)
 * - Delayed expensive operations (prevents watchdog timeout)
 * - Progressive enhancement (basic features first, advanced later)
 */

// ============================================================================
// GLOBAL STATE
// ============================================================================

var SCREEN_WIDTH = 240;
var SCREEN_HEIGHT = 320;

// UI element IDs
var titleLabel = null;
var clockLabel = null;
var weatherLabel = null;
var tempLabel = null;
var displaySyncLabel = null;
var mqttStatusLabel = null;
var mqttMessageLabel = null;
var wsStatusLabel = null;

// Timing control
var lastClockUpdate = 0;
var lastWeatherUpdate = 0;
var lastDisplaySync = 0;
var startTime = 0;

// Update intervals (in milliseconds)
var CLOCK_INTERVAL = 1000;
var WEATHER_INTERVAL = 300000;
var DISPLAY_SYNC_INTERVAL = 5000;

// Initialization delays (prevents watchdog timeout)
var MQTT_INIT_DELAY = 3000;      // Wait 3s before MQTT
var WEATHER_INIT_DELAY = 5000;   // Wait 5s before first weather fetch
var WS_INIT_DELAY = 8000;        // Wait 8s before WebSocket (optional)

// State
var launchCount = 0;
var currentTemp = "--";
var weatherCondition = "Loading...";
var mqttConnected = false;
var wsConnected = false;
var mqttInitialized = false;
var wsInitialized = false;
var weatherInitialized = false;
var myDisplayId = 0;
var totalDisplays = 0;

// Animation state
var titleAnimated = false;
var animationStartTime = 0;

// Configuration
var MQTT_BROKER = "broker.hivemq.com";
var MQTT_PORT = 1883;
var MQTT_TOPIC = "doki/demo/#";
var WEATHER_API_KEY = "3183db8ec2fe4abfa2c133226251310";
var WEATHER_LOCATION = "Mumbai";
var WS_URL = "ws://echo.websocket.org";

// ============================================================================
// LIFECYCLE METHODS
// ============================================================================

function onCreate() {
    log("=== Advanced Demo App Created (Non-Blocking) ===");

    myDisplayId = getDisplayId();
    totalDisplays = getDisplayCount();
    log("Running on Display " + myDisplayId + " of " + totalDisplays);

    // Restore state
    var saved = loadState("launchCount");
    launchCount = saved ? parseInt(saved) : 0;
    launchCount++;
    log("Launch count: " + launchCount);

    // Setup UI (lightweight, non-blocking)
    setupUI();

    log("Advanced Demo onCreate complete");
}

function onStart() {
    log("Advanced Demo started (non-blocking mode)");
    
    // Record start time for delayed initialization
    startTime = millis();
    animationStartTime = millis();

    log("Features will initialize gradually to prevent blocking");
}

function onUpdate() {
    var now = millis();
    var elapsed = now - startTime;

    // === ANIMATION: Title fade-in (immediate, but with delay) ===
    if (!titleAnimated) {
        if (now - animationStartTime > 500) {  // Wait 500ms
            if (titleLabel !== null) {
                setOpacity(titleLabel, 0);
                fadeIn(titleLabel, 2000);  // 2 second fade
                titleAnimated = true;
                log("Title fade-in started");
            }
        }
    }

    // === CLOCK: Update every second ===
    if (now - lastClockUpdate >= CLOCK_INTERVAL) {
        updateClock();
        lastClockUpdate = now;
    }

    // === MQTT: Initialize after delay (non-blocking) ===
    if (!mqttInitialized && elapsed > MQTT_INIT_DELAY) {
        connectMQTT();
        mqttInitialized = true;
    }

    // === WEATHER: Initialize after delay ===
    if (!weatherInitialized && elapsed > WEATHER_INIT_DELAY) {
        updateWeather();
        weatherInitialized = true;
        lastWeatherUpdate = now;
    }

    // === WEATHER: Update periodically ===
    if (weatherInitialized && (now - lastWeatherUpdate >= WEATHER_INTERVAL)) {
        updateWeather();
        lastWeatherUpdate = now;
    }

    // === MULTI-DISPLAY: Sync periodically ===
    if (elapsed > 2000 && (now - lastDisplaySync >= DISPLAY_SYNC_INTERVAL)) {
        syncWithDisplays();
        lastDisplaySync = now;
    }

    // === WEBSOCKET: Initialize after delay (optional, often fails) ===
    if (!wsInitialized && elapsed > WS_INIT_DELAY) {
        connectWebSocket();
        wsInitialized = true;
    }
}

function onPause() {
    log("Advanced Demo paused");
}

function onDestroy() {
    log("Advanced Demo destroyed");

    if (mqttConnected) {
        mqttDisconnect();
        log("MQTT disconnected");
    }

    if (wsConnected) {
        wsDisconnect();
        log("WebSocket disconnected");
    }
}

function onSaveState() {
    log("Saving state - Launch count: " + launchCount);
    saveState("launchCount", launchCount.toString());
    return { launchCount: launchCount };
}

function onRestoreState(state) {
    log("Restoring state");
    var saved = loadState("launchCount");
    if (saved) {
        launchCount = parseInt(saved);
        log("Restored launch count: " + launchCount);
    }
}

// ============================================================================
// UI SETUP
// ============================================================================

function setupUI() {
    setBackgroundColor(0x0a0e27);

    // Title (will animate later)
    titleLabel = createLabel("Advanced Demo", 45, 10);
    setLabelColor(titleLabel, 0x00d4ff);
    setLabelSize(titleLabel, 20);

    // Clock section
    createLabel("CLOCK", 10, 50);
    setLabelColor(0, 0x888888);
    setLabelSize(0, 12);

    clockLabel = createLabel("Starting...", 50, 70);
    setLabelColor(clockLabel, 0xffffff);
    setLabelSize(clockLabel, 24);

    drawRectangle(10, 45, 220, 2, 0x333333);
    drawCircle(30, 80, 3, 0x00d4ff);

    // Weather section
    createLabel("WEATHER", 10, 110);
    setLabelColor(1, 0x888888);
    setLabelSize(1, 12);

    weatherLabel = createLabel("Initializing...", 10, 130);
    setLabelColor(weatherLabel, 0xffaa00);
    setLabelSize(weatherLabel, 14);

    tempLabel = createLabel("--", 10, 150);
    setLabelColor(tempLabel, 0xffffff);
    setLabelSize(tempLabel, 16);

    drawRectangle(10, 105, 220, 2, 0x333333);

    // Multi-Display section
    createLabel("DISPLAYS", 10, 175);
    setLabelColor(2, 0x888888);
    setLabelSize(2, 12);

    displaySyncLabel = createLabel("Initializing...", 10, 195);
    setLabelColor(displaySyncLabel, 0x00ff88);
    setLabelSize(displaySyncLabel, 12);

    drawRectangle(10, 170, 220, 2, 0x333333);

    // MQTT section
    createLabel("MQTT", 10, 215);
    setLabelColor(3, 0x888888);
    setLabelSize(3, 12);

    mqttStatusLabel = createLabel("Waiting...", 10, 235);
    setLabelColor(mqttStatusLabel, 0xaaaaaa);
    setLabelSize(mqttStatusLabel, 12);

    mqttMessageLabel = createLabel("", 10, 250);
    setLabelColor(mqttMessageLabel, 0xaaaaaa);
    setLabelSize(mqttMessageLabel, 12);

    drawRectangle(10, 210, 220, 2, 0x333333);

    // WebSocket section
    createLabel("WEBSOCKET", 10, 270);
    setLabelColor(4, 0x888888);
    setLabelSize(4, 12);

    wsStatusLabel = createLabel("Waiting...", 10, 290);
    setLabelColor(wsStatusLabel, 0xaaaaaa);
    setLabelSize(wsStatusLabel, 12);

    drawRectangle(10, 265, 220, 2, 0x333333);

    log("UI setup complete");
}

// ============================================================================
// CLOCK UPDATE - NON-BLOCKING WITH NTP
// ============================================================================

function updateClock() {
    var time = null;

    try {
        time = getTime();  // May return null if NTP not synced yet
    } catch (e) {
        time = null;
    }

    if (time === null) {
        // NTP not synced yet, show friendly message
        if (clockLabel !== null) {
            var uptime = Math.floor(millis() / 1000);
            if (uptime < 5) {
                updateLabel(clockLabel, "Syncing...");
            } else {
                // Fallback: show uptime
                var s = uptime % 60;
                var m = Math.floor(uptime / 60) % 60;
                var h = Math.floor(uptime / 3600);
                updateLabel(clockLabel, pad(h) + ":" + pad(m) + ":" + pad(s) + " *");
            }
        }
        return;
    }

    // Real NTP time available!
    var timeStr = pad(time.hour) + ":" + pad(time.minute) + ":" + pad(time.second);

    if (clockLabel !== null) {
        updateLabel(clockLabel, timeStr);
    }
}

// ============================================================================
// WEATHER UPDATE
// ============================================================================

function updateWeather() {
    log("Fetching weather for " + WEATHER_LOCATION);

    if (weatherLabel !== null) {
        updateLabel(weatherLabel, "Fetching...");
    }

    var url = "http://api.weatherapi.com/v1/current.json?key=" +
              WEATHER_API_KEY + "&q=" + WEATHER_LOCATION + "&aqi=no";

    var response = httpGet(url);

    if (response) {
        try {
            var data = JSON.parse(response);
            currentTemp = data.current.temp_c.toString();
            weatherCondition = data.current.condition.text;

            log("Weather: " + weatherCondition + ", " + currentTemp + "C");

            if (weatherLabel !== null) {
                updateLabel(weatherLabel, weatherCondition);
            }
            if (tempLabel !== null) {
                updateLabel(tempLabel, currentTemp + " C");
                // Subtle animation
                fadeOut(tempLabel, 150);
                fadeIn(tempLabel, 300);
            }
        } catch (e) {
            log("Error parsing weather: " + e);
            if (weatherLabel !== null) {
                updateLabel(weatherLabel, "Parse Error");
            }
        }
    } else {
        log("Failed to fetch weather");
        if (weatherLabel !== null) {
            updateLabel(weatherLabel, "API Error");
        }
    }
}

// ============================================================================
// MULTI-DISPLAY COORDINATION
// ============================================================================

function syncWithDisplays() {
    if (totalDisplays < 2) {
        if (displaySyncLabel !== null) {
            updateLabel(displaySyncLabel, "Single display");
        }
        return;
    }

    // Send message to other displays
    for (var i = 0; i < totalDisplays; i++) {
        if (i !== myDisplayId) {
            var msg = "Hello from D" + myDisplayId;
            sendToDisplay(i, msg);
        }
    }

    if (displaySyncLabel !== null) {
        updateLabel(displaySyncLabel, "Display " + myDisplayId + "/" + totalDisplays);
    }
}

// ============================================================================
// MQTT INTEGRATION (NON-BLOCKING)
// ============================================================================

function connectMQTT() {
    log("Connecting to MQTT: " + MQTT_BROKER + " (with timeout)");

    if (mqttStatusLabel !== null) {
        updateLabel(mqttStatusLabel, "Connecting...");
        setLabelColor(mqttStatusLabel, 0xffaa00);
    }

    var clientId = "doki-" + myDisplayId + "-" + Math.floor(millis());
    var success = mqttConnect(MQTT_BROKER, MQTT_PORT, clientId);

    if (success) {
        mqttConnected = true;
        log("MQTT connected!");

        mqttSubscribe(MQTT_TOPIC);

        if (mqttStatusLabel !== null) {
            updateLabel(mqttStatusLabel, "Connected");
            setLabelColor(mqttStatusLabel, 0x00ff88);
        }

        mqttPublish("doki/demo/status", "D" + myDisplayId + " online");
    } else {
        mqttConnected = false;
        log("MQTT connection failed (timeout or error)");

        if (mqttStatusLabel !== null) {
            updateLabel(mqttStatusLabel, "Failed");
            setLabelColor(mqttStatusLabel, 0xff4444);
        }
    }
}

// ============================================================================
// WEBSOCKET INTEGRATION (OPTIONAL)
// ============================================================================

function connectWebSocket() {
    log("Connecting to WebSocket: " + WS_URL);

    if (wsStatusLabel !== null) {
        updateLabel(wsStatusLabel, "Connecting...");
        setLabelColor(wsStatusLabel, 0xffaa00);
    }

    var success = wsConnect(WS_URL);

    if (success) {
        wsConnected = true;
        log("WebSocket connected!");

        if (wsStatusLabel !== null) {
            updateLabel(wsStatusLabel, "Connected");
            setLabelColor(wsStatusLabel, 0x00ff88);
        }

        wsSend("Hello from D" + myDisplayId);
    } else {
        wsConnected = false;
        log("WebSocket failed (not supported or error)");

        if (wsStatusLabel !== null) {
            updateLabel(wsStatusLabel, "Not supported");
            setLabelColor(wsStatusLabel, 0x888888);
        }
    }
}

// ============================================================================
// HELPER FUNCTIONS
// ============================================================================


function pad(num) {
    return num < 10 ? "0" + num : num.toString();
}

// ============================================================================
// END
// ============================================================================

log("Advanced demo V3 (non-blocking) loaded");