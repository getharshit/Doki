/**
 * @file simple_http_server.cpp
 * @brief Implementation of Simple HTTP Server
 */

#include "doki/simple_http_server.h"
#include <WiFi.h>

namespace Doki {

// Static member initialization
AsyncWebServer* SimpleHttpServer::_server = nullptr;
bool SimpleHttpServer::_running = false;
bool (*SimpleHttpServer::_loadAppCallback)(uint8_t, const String&) = nullptr;
void (*SimpleHttpServer::_statusCallback)(uint8_t, String&, uint32_t&) = nullptr;

bool SimpleHttpServer::begin(uint16_t port) {
    if (_running) {
        Serial.println("[SimpleHTTP] Server already running");
        return true;
    }

    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("[SimpleHTTP] Error: WiFi not connected");
        return false;
    }

    Serial.printf("[SimpleHTTP] Starting server on port %d...\n", port);

    _server = new AsyncWebServer(port);

    // API: Get available apps
    _server->on("/api/apps", HTTP_GET, handleGetApps);

    // API: Load app on display
    _server->on("/api/load", HTTP_POST, handleLoadApp);

    // API: Get displays status
    _server->on("/api/status", HTTP_GET, handleGetStatus);

    // Dashboard HTML
    _server->on("/", HTTP_GET, handleDashboard);

    // Enable CORS
    DefaultHeaders::Instance().addHeader("Access-Control-Allow-Origin", "*");

    _server->begin();
    _running = true;

    Serial.printf("[SimpleHTTP] ✓ Server started at http://%s\n", WiFi.localIP().toString().c_str());
    return true;
}

void SimpleHttpServer::stop() {
    if (!_running || !_server) return;

    Serial.println("[SimpleHTTP] Stopping server...");
    _server->end();
    delete _server;
    _server = nullptr;
    _running = false;
}

bool SimpleHttpServer::isRunning() {
    return _running;
}

void SimpleHttpServer::setLoadAppCallback(bool (*callback)(uint8_t, const String&)) {
    _loadAppCallback = callback;
}

void SimpleHttpServer::setStatusCallback(void (*callback)(uint8_t, String&, uint32_t&)) {
    _statusCallback = callback;
}

void SimpleHttpServer::handleGetApps(AsyncWebServerRequest* request) {
    JsonDocument doc;
    JsonArray apps = doc["apps"].to<JsonArray>();

    // List of available apps
    apps.add("clock");
    apps.add("weather");
    apps.add("sysinfo");
    apps.add("hello");
    apps.add("goodbye");
    apps.add("blank");

    String response;
    serializeJson(doc, response);
    request->send(200, "application/json", response);
}

void SimpleHttpServer::handleLoadApp(AsyncWebServerRequest* request) {
    if (!_loadAppCallback) {
        request->send(500, "application/json", "{\"error\":\"No callback set\"}");
        return;
    }

    if (!request->hasParam("display") || !request->hasParam("app")) {
        request->send(400, "application/json", "{\"error\":\"Missing display or app parameter\"}");
        return;
    }

    int displayId = request->getParam("display")->value().toInt();
    String appId = request->getParam("app")->value();

    if (displayId < 0 || displayId >= 2) {
        request->send(400, "application/json", "{\"error\":\"Invalid display ID\"}");
        return;
    }

    bool success = _loadAppCallback(displayId, appId);

    if (success) {
        request->send(200, "application/json", "{\"success\":true}");
    } else {
        request->send(500, "application/json", "{\"error\":\"Failed to load app\"}");
    }
}

void SimpleHttpServer::handleGetStatus(AsyncWebServerRequest* request) {
    if (!_statusCallback) {
        request->send(500, "application/json", "{\"error\":\"No callback set\"}");
        return;
    }

    JsonDocument doc;
    JsonArray disps = doc["displays"].to<JsonArray>();

    for (int i = 0; i < 2; i++) {
        String appId;
        uint32_t uptime = 0;
        _statusCallback(i, appId, uptime);

        JsonObject d = disps.add<JsonObject>();
        d["id"] = i;
        d["app"] = appId;
        d["uptime"] = uptime;
    }

    String response;
    serializeJson(doc, response);
    request->send(200, "application/json", response);
}

void SimpleHttpServer::handleDashboard(AsyncWebServerRequest* request) {
    request->send(200, "text/html", generateDashboardHTML());
}

String SimpleHttpServer::generateDashboardHTML() {
    return R"rawliteral(
<!DOCTYPE html>
<html>
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Doki OS Dashboard</title>
    <style>
        * { margin: 0; padding: 0; box-sizing: border-box; }
        body {
            font-family: -apple-system, BlinkMacSystemFont, 'Segoe UI', sans-serif;
            background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
            min-height: 100vh;
            padding: 20px;
        }
        .container {
            max-width: 1200px;
            margin: 0 auto;
            background: white;
            border-radius: 16px;
            padding: 32px;
            box-shadow: 0 8px 32px rgba(0,0,0,0.2);
        }
        h1 { color: #667eea; margin-bottom: 32px; }
        .displays-grid {
            display: grid;
            grid-template-columns: repeat(auto-fit, minmax(400px, 1fr));
            gap: 24px;
            margin-bottom: 32px;
        }
        .display-card {
            background: #f9fafb;
            border-radius: 12px;
            padding: 24px;
            border: 3px solid #e5e7eb;
            cursor: pointer;
            transition: all 0.2s;
        }
        .display-card:hover { transform: translateY(-2px); }
        .display-card.selected { border-color: #667eea; box-shadow: 0 0 0 3px rgba(102, 126, 234, 0.2); }
        .display-card.active { border-color: #10b981; }
        .display-title {
            font-size: 1.5em;
            font-weight: 600;
            color: #111827;
            margin-bottom: 16px;
        }
        .current-app {
            background: white;
            padding: 16px;
            border-radius: 8px;
            margin-bottom: 16px;
        }
        .app-name { font-size: 1.2em; font-weight: 600; color: #667eea; }
        .app-uptime { color: #6b7280; font-size: 0.9em; margin-top: 4px; }
        .apps-section { margin-top: 32px; }
        .apps-grid {
            display: grid;
            grid-template-columns: repeat(auto-fill, minmax(150px, 1fr));
            gap: 12px;
        }
        .app-btn {
            padding: 16px;
            background: #667eea;
            color: white;
            border: none;
            border-radius: 8px;
            cursor: pointer;
            font-size: 1em;
            font-weight: 600;
            transition: all 0.2s;
        }
        .app-btn:hover { background: #5568d3; transform: translateY(-2px); }
        .message {
            padding: 12px;
            border-radius: 8px;
            margin-bottom: 16px;
            display: none;
        }
        .message.success { background: #d1fae5; color: #065f46; display: block; }
        .message.error { background: #fee2e2; color: #991b1b; display: block; }
    </style>
</head>
<body>
    <div class="container">
        <h1>🎨 Doki OS Dashboard</h1>
        <div id="message" class="message"></div>
        <div class="displays-grid" id="displays"></div>
        <div class="apps-section">
            <h2>📱 Available Apps</h2>
            <p style="color: #6b7280; margin-bottom: 16px;">Click a display above, then click an app to load it</p>
            <div class="apps-grid" id="apps"></div>
        </div>
    </div>
    <script>
        let selectedDisplay = 0;

        function showMessage(text, type) {
            const msg = document.getElementById('message');
            msg.textContent = text;
            msg.className = 'message ' + type;
            setTimeout(function() { msg.style.display = 'none'; }, 3000);
        }

        function selectDisplay(id) {
            selectedDisplay = id;
            loadStatus();
        }

        function loadApp(appId) {
            const url = '/api/load?display=' + selectedDisplay + '&app=' + appId;
            fetch(url, { method: 'POST' })
                .then(function(res) { return res.json(); })
                .then(function(data) {
                    if (data.success) {
                        showMessage('Loaded ' + appId + ' on Display ' + selectedDisplay, 'success');
                        loadStatus();
                    } else {
                        showMessage('Failed to load app', 'error');
                    }
                })
                .catch(function(err) {
                    showMessage('Error: ' + err.message, 'error');
                });
        }

        function loadStatus() {
            fetch('/api/status')
                .then(function(res) { return res.json(); })
                .then(function(data) {
                    const displaysDiv = document.getElementById('displays');
                    let html = '';
                    for (let i = 0; i < data.displays.length; i++) {
                        const d = data.displays[i];
                        const activeClass = d.app ? 'active' : '';
                        const selectedClass = selectedDisplay === i ? 'selected' : '';
                        const appName = d.app || 'No app loaded';
                        html += '<div class="display-card ' + activeClass + ' ' + selectedClass + '" onclick="selectDisplay(' + i + ')">';
                        html += '<div class="display-title">Display ' + d.id + '</div>';
                        html += '<div class="current-app">';
                        html += '<div class="app-name">' + appName + '</div>';
                        html += '<div class="app-uptime">Uptime: ' + d.uptime + 's</div>';
                        html += '</div></div>';
                    }
                    displaysDiv.innerHTML = html;
                });
        }

        function loadApps() {
            fetch('/api/apps')
                .then(function(res) { return res.json(); })
                .then(function(data) {
                    const appsDiv = document.getElementById('apps');
                    let html = '';
                    for (let i = 0; i < data.apps.length; i++) {
                        const app = data.apps[i];
                        html += '<button class="app-btn" onclick="loadApp(\'' + app + '\')">' + app + '</button>';
                    }
                    appsDiv.innerHTML = html;
                });
        }

        loadStatus();
        loadApps();
        setInterval(loadStatus, 2000);
    </script>
</body>
</html>
)rawliteral";
}

} // namespace Doki
