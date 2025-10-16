/**
 * @file setup_portal.cpp
 * @brief Implementation of SetupPortal
 */

#include "doki/setup_portal.h"
#include "doki/storage_manager.h"
#include "doki/wifi_manager.h"
#include <esp_task_wdt.h>

namespace Doki {

// ========================================
// Static Member Initialization
// ========================================

AsyncWebServer* SetupPortal::_server = nullptr;
DNSServer* SetupPortal::_dnsServer = nullptr;
bool SetupPortal::_running = false;

// ========================================
// Public Methods
// ========================================

bool SetupPortal::begin(uint16_t port) {
    if (_running) {
        Serial.println("[SetupPortal] Already running");
        return true;
    }

    Serial.println("╔═══════════════════════════════════════════════════╗");
    Serial.println("║           Setup Portal Starting                    ║");
    Serial.println("╚═══════════════════════════════════════════════════╝");

    // Create DNS server for captive portal
    _dnsServer = new DNSServer();
    _dnsServer->start(DNS_PORT, "*", WiFi.softAPIP());

    // Create web server
    _server = new AsyncWebServer(port);

    // Setup routes
    _server->on("/", HTTP_GET, handleRoot);
    _server->on("/setup", HTTP_GET, handleSetup);
    _server->on("/save", HTTP_POST, handleSaveWiFi);
    _server->on("/status", HTTP_GET, handleStatus);
    _server->onNotFound(handleNotFound);

    // Start server
    _server->begin();
    _running = true;

    Serial.println("[SetupPortal] ✓ Setup portal started");
    Serial.printf("[SetupPortal] URL: %s\n", getSetupURL().c_str());
    Serial.println("[SetupPortal] Connect to AP and navigate to any URL");

    return true;
}

void SetupPortal::stop() {
    if (!_running) return;

    Serial.println("[SetupPortal] Stopping setup portal...");

    if (_server) {
        _server->end();
        delete _server;
        _server = nullptr;
    }

    if (_dnsServer) {
        _dnsServer->stop();
        delete _dnsServer;
        _dnsServer = nullptr;
    }

    _running = false;
    Serial.println("[SetupPortal] ✓ Setup portal stopped");
}

void SetupPortal::update() {
    if (!_running || !_dnsServer) return;
    _dnsServer->processNextRequest();
}

bool SetupPortal::isRunning() {
    return _running;
}

String SetupPortal::getSetupURL() {
    return "http://" + WiFi.softAPIP().toString() + "/setup";
}

// ========================================
// HTTP Handlers
// ========================================

void SetupPortal::handleRoot(AsyncWebServerRequest* request) {
    // Redirect to setup page
    request->redirect("/setup");
}

void SetupPortal::handleSetup(AsyncWebServerRequest* request) {
    Serial.printf("[SetupPortal] Setup page requested from %s\n",
                  request->client()->remoteIP().toString().c_str());

    request->send(200, "text/html", generateSetupPage());
}

// Network scanning removed - not needed
// void SetupPortal::handleScanNetworks(AsyncWebServerRequest* request) {
//     // Removed to simplify setup process
// }

void SetupPortal::handleSaveWiFi(AsyncWebServerRequest* request) {
    Serial.println("[SetupPortal] WiFi configuration received");

    // Get SSID and password from POST data
    if (!request->hasParam("ssid", true) || !request->hasParam("password", true)) {
        JsonDocument doc;
        doc["error"] = "Missing SSID or password";
        sendJsonResponse(request, 400, doc);
        return;
    }

    String ssid = request->getParam("ssid", true)->value();
    String password = request->getParam("password", true)->value();

    // Trim whitespace from SSID and password
    ssid.trim();
    password.trim();

    // Validate SSID
    if (ssid.isEmpty()) {
        JsonDocument doc;
        doc["error"] = "SSID cannot be empty";
        sendJsonResponse(request, 400, doc);
        return;
    }

    Serial.printf("[SetupPortal] Saving credentials: SSID='%s'\n", ssid.c_str());

    // Save to storage
    if (!StorageManager::saveWiFiCredentials(ssid, password)) {
        JsonDocument doc;
        doc["error"] = "Failed to save credentials";
        sendJsonResponse(request, 500, doc);
        return;
    }

    // Send success response
    JsonDocument doc;
    doc["success"] = true;
    doc["message"] = "WiFi configured successfully! Device will restart in 3 seconds...";
    sendJsonResponse(request, 200, doc);

    // Restart device after a short delay
    Serial.println("[SetupPortal] ✓ Credentials saved, restarting in 3 seconds...");

    // Schedule restart
    static bool restartScheduled = false;
    if (!restartScheduled) {
        restartScheduled = true;
        // Use a timer to restart after sending response
        auto restartTask = []() {
            delay(3000);
            ESP.restart();
        };
        xTaskCreate([](void* param) {
            delay(3000);
            ESP.restart();
        }, "Restart", 2048, nullptr, 1, nullptr);
    }
}

void SetupPortal::handleStatus(AsyncWebServerRequest* request) {
    JsonDocument doc;
    doc["ap_ip"] = WiFi.softAPIP().toString();
    doc["ap_clients"] = WiFi.softAPgetStationNum();
    doc["has_credentials"] = StorageManager::hasWiFiCredentials();

    sendJsonResponse(request, 200, doc);
}

void SetupPortal::handleNotFound(AsyncWebServerRequest* request) {
    // Captive portal: redirect all unknown requests to setup page
    request->redirect("/setup");
}

// ========================================
// HTML Page Generators
// ========================================

String SetupPortal::generateSetupPage() {
    return R"rawliteral(
<!DOCTYPE html>
<html>
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Doki OS - WiFi Setup</title>
    <style>
        * { margin: 0; padding: 0; box-sizing: border-box; }
        body {
            font-family: -apple-system, BlinkMacSystemFont, 'Segoe UI', sans-serif;
            background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
            min-height: 100vh;
            display: flex;
            justify-content: center;
            align-items: center;
            padding: 20px;
        }
        .container {
            background: white;
            border-radius: 16px;
            padding: 32px;
            box-shadow: 0 8px 32px rgba(0,0,0,0.2);
            max-width: 480px;
            width: 100%;
        }
        h1 {
            color: #667eea;
            margin-bottom: 8px;
            font-size: 2em;
        }
        .subtitle {
            color: #666;
            margin-bottom: 24px;
            font-size: 0.95em;
        }
        .info {
            background: #eff6ff;
            border-left: 4px solid #667eea;
            padding: 12px;
            margin-bottom: 24px;
            border-radius: 4px;
            font-size: 0.9em;
            color: #1e40af;
        }
        label {
            display: block;
            margin-bottom: 8px;
            color: #333;
            font-weight: 600;
        }
        input {
            width: 100%;
            padding: 12px;
            border: 2px solid #e5e7eb;
            border-radius: 8px;
            font-size: 1em;
            margin-bottom: 16px;
            transition: border-color 0.2s;
        }
        input:focus {
            outline: none;
            border-color: #667eea;
        }
        button {
            width: 100%;
            padding: 14px;
            background: #667eea;
            color: white;
            border: none;
            border-radius: 8px;
            font-size: 1.05em;
            font-weight: 600;
            cursor: pointer;
            transition: background 0.2s;
        }
        button:hover { background: #5568d3; }
        button:disabled {
            background: #9ca3af;
            cursor: not-allowed;
        }
        .message {
            padding: 12px;
            border-radius: 8px;
            margin-bottom: 16px;
            display: none;
        }
        .message.success {
            background: #d1fae5;
            color: #065f46;
            display: block;
        }
        .message.error {
            background: #fee2e2;
            color: #991b1b;
            display: block;
        }
    </style>
</head>
<body>
    <div class="container">
        <h1>🎨 Doki OS</h1>
        <p class="subtitle">WiFi Configuration</p>

        <div class="info">
            💡 Enter your WiFi credentials. Leave password blank for open networks.
        </div>

        <div id="message" class="message"></div>

        <form id="wifiForm" onsubmit="saveWiFi(event)">
            <label for="ssid">Network Name (SSID)</label>
            <input type="text" id="ssid" name="ssid" required placeholder="Enter WiFi network name" autocomplete="off">

            <label for="password">Password (optional)</label>
            <input type="password" id="password" name="password" placeholder="Leave blank for open WiFi" autocomplete="off">

            <button type="submit" id="submitBtn">Connect to WiFi</button>
        </form>
    </div>

    <script>
        function showMessage(text, type) {
            const msg = document.getElementById('message');
            msg.textContent = text;
            msg.className = 'message ' + type;
        }

        async function saveWiFi(event) {
            event.preventDefault();

            const ssid = document.getElementById('ssid').value.trim();
            const password = document.getElementById('password').value.trim();
            const btn = document.getElementById('submitBtn');

            if (!ssid) {
                showMessage('Please enter a network name', 'error');
                return;
            }

            btn.disabled = true;
            btn.textContent = 'Connecting...';
            showMessage('Saving WiFi credentials...', 'success');

            try {
                const formData = new FormData();
                formData.append('ssid', ssid);
                formData.append('password', password || '');

                const res = await fetch('/save', {
                    method: 'POST',
                    body: formData
                });

                const data = await res.json();

                if (data.success) {
                    showMessage('✓ WiFi configured! Restarting...', 'success');
                } else {
                    showMessage('Error: ' + (data.error || 'Unknown error'), 'error');
                    btn.disabled = false;
                    btn.textContent = 'Connect to WiFi';
                }

            } catch (err) {
                showMessage('Connection error. Try again.', 'error');
                btn.disabled = false;
                btn.textContent = 'Connect to WiFi';
            }
        }
    </script>
</body>
</html>
)rawliteral";
}

// ========================================
// Helper Functions
// ========================================

void SetupPortal::sendJsonResponse(AsyncWebServerRequest* request,
                                     int statusCode,
                                     const JsonDocument& doc) {
    String response;
    serializeJson(doc, response);
    request->send(statusCode, "application/json", response);
}

} // namespace Doki
