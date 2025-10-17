/**
 * WebSocket Diagnostics App for Doki OS
 *
 * Purpose: Test and diagnose WebSocket connectivity
 * Tests: Connection, send/receive, error handling
 */

// ============================================================================
// GLOBAL STATE
// ============================================================================

// UI Elements
var titleLabel = null;
var statusLabel = null;
var serverLabel = null;
var logLabel1 = null;
var logLabel2 = null;
var logLabel3 = null;
var logLabel4 = null;
var logLabel5 = null;
var testCountLabel = null;

// State
var wsConnected = false;
var testServers = [
    "wss://echo.websocket.org/",            // Secure WebSocket (wss://) on port 443
    "ws://ws.postman-echo.com/raw"          // Fallback non-secure server
];
var currentServerIndex = 0;
var currentServer = testServers[0];
var connectionAttempts = 0;
var messagesReceived = 0;
var messagesSent = 0;
var lastUpdate = 0;

// Test sequence
var testPhase = 0;
var testStartTime = 0;
var autoTestEnabled = true;

// Log buffer (circular)
var logMessages = [];
var maxLogMessages = 5;

// ============================================================================
// LIFECYCLE
// ============================================================================

function onCreate() {
    logMessage("WebSocket Diagnostics App Created");
    setupUI();
}

function onStart() {
    logMessage("WebSocket Diagnostics Started");
    testStartTime = millis();

    // Start automatic testing sequence
    if (autoTestEnabled) {
        logMessage("Starting automatic test sequence...");
    }
}

function onUpdate() {
    var now = millis();
    var elapsed = now - testStartTime;

    // Update every 100ms
    if (now - lastUpdate < 100) {
        return;
    }
    lastUpdate = now;

    // Automatic test sequence
    if (autoTestEnabled) {
        if (testPhase === 0 && elapsed > 2000) {
            // Phase 0: Wait 2s, then connect
            testPhase = 1;
            attemptConnection(currentServer);
        }
        else if (testPhase === 1 && wsConnected && elapsed > 5000) {
            // Phase 1: If connected after 5s, send test message
            testPhase = 2;
            sendTestMessage("Hello from Doki OS!");
        }
        else if (testPhase === 1 && !wsConnected && elapsed > 10000) {
            // Phase 1: If not connected after 10s, try next server
            currentServerIndex++;
            if (currentServerIndex < testServers.length) {
                testPhase = 3;
                currentServer = testServers[currentServerIndex];
                logMessage("Trying server " + (currentServerIndex + 1) + "...");
                attemptConnection(currentServer);
            } else {
                testPhase = 5;
                logMessage("All servers failed");
                autoTestEnabled = false;
            }
        }
        else if (testPhase === 2 && elapsed > 8000) {
            // Phase 2: Send another test after 8s
            testPhase = 4;
            sendTestMessage("Test message #2");
        }
        else if (testPhase === 3 && wsConnected && elapsed > 15000) {
            // Phase 3: Server connected, send test
            testPhase = 4;
            sendTestMessage("Connected to server!");
        }
        else if (testPhase === 3 && !wsConnected && elapsed > 20000) {
            // Phase 3: Current server failed, try next
            currentServerIndex++;
            if (currentServerIndex < testServers.length) {
                currentServer = testServers[currentServerIndex];
                logMessage("Trying server " + (currentServerIndex + 1) + "...");
                attemptConnection(currentServer);
            } else {
                testPhase = 5;
                logMessage("All servers failed");
                autoTestEnabled = false;
            }
        }
        else if (testPhase === 4 && elapsed > 20000) {
            // Phase 4: Test complete, just maintain connection
            testPhase = 5;
            logMessage("Automatic testing complete");
            autoTestEnabled = false;
        }
    }

    // Update test counter
    updateTestCounter();
}

function onPause() {
    logMessage("WebSocket Diagnostics Paused");
}

function onDestroy() {
    logMessage("WebSocket Diagnostics Destroyed");
    if (wsConnected) {
        wsDisconnect();
    }
}

function onSaveState() {
    return {
        attempts: connectionAttempts,
        sent: messagesSent,
        received: messagesReceived
    };
}

function onRestoreState(state) {
    logMessage("State restored");
}

// ============================================================================
// UI SETUP
// ============================================================================

function setupUI() {
    // Dark blue background
    setBackgroundColor(0x0f172a);

    // Title
    titleLabel = createLabel("WebSocket Test", 50, 10);
    setLabelColor(titleLabel, 0x60a5fa);
    setLabelSize(titleLabel, 20);

    // Divider
    drawRectangle(10, 35, 220, 2, 0x334155);

    // Status section
    createLabel("STATUS", 10, 45);
    setLabelColor(0, 0x94a3b8);
    setLabelSize(0, 12);

    statusLabel = createLabel("Initializing...", 10, 65);
    setLabelColor(statusLabel, 0xfbbf24);
    setLabelSize(statusLabel, 14);

    // Server section
    createLabel("SERVER", 10, 90);
    setLabelColor(1, 0x94a3b8);
    setLabelSize(1, 12);

    serverLabel = createLabel("echo.websocket.org", 10, 110);
    setLabelColor(serverLabel, 0xa78bfa);
    setLabelSize(serverLabel, 12);

    // Divider
    drawRectangle(10, 130, 220, 2, 0x334155);

    // Log section
    createLabel("MESSAGE LOG", 10, 140);
    setLabelColor(2, 0x94a3b8);
    setLabelSize(2, 12);

    logLabel1 = createLabel("", 10, 160);
    setLabelColor(logLabel1, 0xe2e8f0);
    setLabelSize(logLabel1, 12);

    logLabel2 = createLabel("", 10, 175);
    setLabelColor(logLabel2, 0xe2e8f0);
    setLabelSize(logLabel2, 12);

    logLabel3 = createLabel("", 10, 190);
    setLabelColor(logLabel3, 0xe2e8f0);
    setLabelSize(logLabel3, 12);

    logLabel4 = createLabel("", 10, 205);
    setLabelColor(logLabel4, 0xe2e8f0);
    setLabelSize(logLabel4, 12);

    logLabel5 = createLabel("", 10, 220);
    setLabelColor(logLabel5, 0xe2e8f0);
    setLabelSize(logLabel5, 12);

    // Divider
    drawRectangle(10, 240, 220, 2, 0x334155);

    // Test counter
    createLabel("TEST STATS", 10, 250);
    setLabelColor(3, 0x94a3b8);
    setLabelSize(3, 12);

    testCountLabel = createLabel("Attempts: 0 | Sent: 0 | Recv: 0", 10, 270);
    setLabelColor(testCountLabel, 0x10b981);
    setLabelSize(testCountLabel, 12);

    // Decorative elements
    drawCircle(220, 15, 5, 0x60a5fa);
    drawCircle(15, 15, 3, 0xa78bfa);

    logMessage("UI initialized");
}

// ============================================================================
// WEBSOCKET FUNCTIONS
// ============================================================================

function attemptConnection(url) {
    connectionAttempts++;
    logMessage("Connecting to: " + url);

    updateStatus("Connecting...", 0xfbbf24);
    updateServer(url);

    var success = wsConnect(url);

    if (success) {
        wsConnected = true;
        logMessage("✓ Connected successfully!");
        updateStatus("Connected", 0x10b981);

        // Setup message handler
        wsOnMessage(function(msg) {
            messagesReceived++;
            logMessage("← Received: " + msg);
        });
    } else {
        wsConnected = false;
        logMessage("✗ Connection failed");
        updateStatus("Failed", 0xef4444);
    }
}

function sendTestMessage(message) {
    if (!wsConnected) {
        logMessage("✗ Cannot send - not connected");
        return;
    }

    messagesSent++;
    logMessage("→ Sending: " + message);

    var success = wsSend(message);
    if (!success) {
        logMessage("✗ Send failed");
    }
}

function disconnectWebSocket() {
    if (wsConnected) {
        wsDisconnect();
        wsConnected = false;
        logMessage("Disconnected");
        updateStatus("Disconnected", 0x64748b);
    }
}

// ============================================================================
// UI UPDATES
// ============================================================================

function updateStatus(status, color) {
    if (statusLabel !== null) {
        updateLabel(statusLabel, status);
        setLabelColor(statusLabel, color);
    }
}

function updateServer(server) {
    if (serverLabel !== null) {
        // Truncate if too long
        var displayServer = server;
        if (server.length > 25) {
            displayServer = server.substring(0, 22) + "...";
        }
        updateLabel(serverLabel, displayServer);
    }
}

function updateTestCounter() {
    if (testCountLabel !== null) {
        var text = "Tries:" + connectionAttempts + " Sent:" + messagesSent + " Recv:" + messagesReceived;
        updateLabel(testCountLabel, text);
    }
}

function refreshLog() {
    // Update log labels with latest messages
    var labels = [logLabel1, logLabel2, logLabel3, logLabel4, logLabel5];

    for (var i = 0; i < maxLogMessages; i++) {
        if (labels[i] !== null) {
            if (i < logMessages.length) {
                updateLabel(labels[i], logMessages[i]);
            } else {
                updateLabel(labels[i], "");
            }
        }
    }
}

// ============================================================================
// LOGGING
// ============================================================================

function logMessage(message) {
    // Add to serial (using 'log' function from Doki OS)
    log("[WebSocket Test] " + message);

    // Add timestamp
    var timestamp = "[" + Math.floor(millis() / 1000) + "s]";
    var logEntry = timestamp + " " + message;

    // Truncate if too long
    if (logEntry.length > 30) {
        logEntry = logEntry.substring(0, 27) + "...";
    }

    // Add to circular buffer
    logMessages.unshift(logEntry);  // Add to front

    // Keep only last N messages
    if (logMessages.length > maxLogMessages) {
        logMessages.pop();  // Remove oldest
    }

    // Refresh display
    refreshLog();
}

// ============================================================================
// END
// ============================================================================

logMessage("WebSocket diagnostic app loaded");
