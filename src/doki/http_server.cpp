/**
 * @file http_server.cpp
 * @brief Implementation of HTTP Server for Doki OS
 */

#include "doki/http_server.h"
#include "doki/app_manager.h"
#include "doki/memory_manager.h"
#include "doki/task_scheduler.h"
#include <WiFi.h>

namespace Doki {

// ========================================
// Static Member Initialization
// ========================================

AsyncWebServer* HttpServer::_server = nullptr;
bool HttpServer::_running = false;

// ========================================
// Public Methods
// ========================================

bool HttpServer::begin(uint16_t port) {
    if (_running) {
        Serial.println("[HttpServer] Server already running");
        return true;
    }
    
    // Check WiFi connection
    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("[HttpServer] Error: WiFi not connected");
        return false;
    }
    
    Serial.printf("[HttpServer] Starting server on port %d...\n", port);
    
    // Create server instance
    _server = new AsyncWebServer(port);
    
    // ========================================
    // API Endpoints
    // ========================================
    
    // POST /api/app/load?id=<app_id>
    _server->on("/api/app/load", HTTP_POST, handleLoadApp);
    
    // POST /api/app/unload
    _server->on("/api/app/unload", HTTP_POST, handleUnloadApp);
    
    // GET /api/app/current
    _server->on("/api/app/current", HTTP_GET, handleGetCurrentApp);
    
    // GET /api/app/list
    _server->on("/api/app/list", HTTP_GET, handleListApps);
    
    // GET /api/system/status
    _server->on("/api/system/status", HTTP_GET, handleSystemStatus);
    
    // GET /dashboard (web UI)
    _server->on("/", HTTP_GET, handleDashboard);
    _server->on("/dashboard", HTTP_GET, handleDashboard);
    
    // 404 handler
    _server->onNotFound(handleNotFound);
    
    // Enable CORS (allow requests from any origin)
    DefaultHeaders::Instance().addHeader("Access-Control-Allow-Origin", "*");
    DefaultHeaders::Instance().addHeader("Access-Control-Allow-Methods", "GET, POST, PUT, DELETE, OPTIONS");
    DefaultHeaders::Instance().addHeader("Access-Control-Allow-Headers", "Content-Type");
    
    // Start server
    _server->begin();
    _running = true;
    
    Serial.printf("[HttpServer] ✓ Server started at %s\n", getServerUrl().c_str());
    Serial.println("[HttpServer] Available endpoints:");
    Serial.println("  POST /api/app/load?id=<app_id>");
    Serial.println("  POST /api/app/unload");
    Serial.println("  GET  /api/app/current");
    Serial.println("  GET  /api/app/list");
    Serial.println("  GET  /api/system/status");
    Serial.println("  GET  /dashboard");
    
    return true;
}

void HttpServer::stop() {
    if (!_running || !_server) {
        return;
    }
    
    Serial.println("[HttpServer] Stopping server...");
    _server->end();
    delete _server;
    _server = nullptr;
    _running = false;
}

bool HttpServer::isRunning() {
    return _running;
}

String HttpServer::getServerUrl() {
    if (WiFi.status() != WL_CONNECTED) {
        return "http://not-connected";
    }
    return "http://" + WiFi.localIP().toString();
}

// ========================================
// API Endpoint Handlers
// ========================================

void HttpServer::handleLoadApp(AsyncWebServerRequest* request) {
    // Get app ID from query parameter
    if (!request->hasParam("id")) {
        sendErrorResponse(request, 400, "Missing 'id' parameter");
        return;
    }
    
    String appId = request->getParam("id")->value();
    
    Serial.printf("[HttpServer] API: Load app '%s'\n", appId.c_str());
    
    // Check if app is registered
    if (!AppManager::isAppRegistered(appId.c_str())) {
        sendErrorResponse(request, 404, "App not found");
        return;
    }
    
    // Load the app
    uint32_t startTime = millis();
    bool success = AppManager::loadApp(appId.c_str());
    uint32_t loadTime = millis() - startTime;
    
    if (success) {
        // Create JSON response
        JsonDocument doc;
        doc["status"] = "success";
        doc["app"] = appId;
        doc["load_time_ms"] = loadTime;
        
        sendJsonResponse(request, 200, doc);
    } else {
        sendErrorResponse(request, 500, "Failed to load app");
    }
}

void HttpServer::handleUnloadApp(AsyncWebServerRequest* request) {
    Serial.println("[HttpServer] API: Unload current app");
    
    if (!AppManager::isAppRunning()) {
        sendErrorResponse(request, 400, "No app currently running");
        return;
    }
    
    const char* appId = AppManager::getCurrentAppId();
    SystemMemory memBefore = MemoryManager::getSystemMemory();
    
    bool success = AppManager::unloadCurrentApp();
    
    if (success) {
        SystemMemory memAfter = MemoryManager::getSystemMemory();
        size_t freedMemory = memAfter.freeHeap - memBefore.freeHeap;
        
        JsonDocument doc;
        doc["status"] = "success";
        doc["unloaded_app"] = appId;
        doc["freed_memory"] = freedMemory;
        
        sendJsonResponse(request, 200, doc);
    } else {
        sendErrorResponse(request, 500, "Failed to unload app");
    }
}

void HttpServer::handleGetCurrentApp(AsyncWebServerRequest* request) {
    Serial.println("[HttpServer] API: Get current app");
    
    JsonDocument doc;
    
    if (AppManager::isAppRunning()) {
        DokiApp* app = AppManager::getCurrentApp();
        
        doc["app_id"] = AppManager::getCurrentAppId();
        doc["app_name"] = app->getName();
        doc["uptime_seconds"] = AppManager::getAppUptime() / 1000;
        doc["uptime_ms"] = AppManager::getAppUptime();
        doc["running"] = true;
    } else {
        doc["running"] = false;
        doc["message"] = "No app currently running";
    }
    
    sendJsonResponse(request, 200, doc);
}

void HttpServer::handleListApps(AsyncWebServerRequest* request) {
    Serial.println("[HttpServer] API: List apps");
    
    JsonDocument doc;
    JsonArray apps = doc["apps"].to<JsonArray>();
    
    std::vector<AppRegistration> registeredApps = AppManager::getRegisteredApps();
    const char* currentId = AppManager::getCurrentAppId();
    
    for (const auto& reg : registeredApps) {
        JsonObject app = apps.add<JsonObject>();
        app["id"] = reg.id;
        app["name"] = reg.name;
        app["description"] = reg.description;
        app["loaded"] = (strcmp(reg.id.c_str(), currentId) == 0);
    }
    
    doc["total"] = registeredApps.size();
    
    sendJsonResponse(request, 200, doc);
}

void HttpServer::handleSystemStatus(AsyncWebServerRequest* request) {
    Serial.println("[HttpServer] API: System status");
    
    SystemMemory mem = MemoryManager::getSystemMemory();
    
    JsonDocument doc;
    
    // System info
    doc["doki_version"] = "0.1.0";
    doc["uptime_seconds"] = millis() / 1000;
    
    // WiFi info
    JsonObject wifi = doc["wifi"].to<JsonObject>();
    wifi["connected"] = WiFi.status() == WL_CONNECTED;
    wifi["ssid"] = WiFi.SSID();
    wifi["rssi"] = WiFi.RSSI();
    wifi["ip"] = WiFi.localIP().toString();
    
    // Memory info
    JsonObject memory = doc["memory"].to<JsonObject>();
    memory["free_heap"] = mem.freeHeap;
    memory["total_heap"] = mem.totalHeap;
    memory["heap_usage_percent"] = (int)(mem.heapUsagePercent * 100);
    memory["free_psram"] = mem.freePsram;
    memory["total_psram"] = mem.totalPsram;
    memory["psram_usage_percent"] = (int)(mem.psramUsagePercent * 100);
    
    // Current app info
    if (AppManager::isAppRunning()) {
        doc["current_app"] = AppManager::getCurrentAppId();
        doc["app_uptime_seconds"] = AppManager::getAppUptime() / 1000;
    } else {
        doc["current_app"] = nullptr;
    }
    
    // Task info
    doc["active_tasks"] = TaskScheduler::getActiveTaskCount();
    
    sendJsonResponse(request, 200, doc);
}

void HttpServer::handleDashboard(AsyncWebServerRequest* request) {
    Serial.println("[HttpServer] Serving dashboard");
    
    // Simple HTML dashboard
    String html = R"rawliteral(
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
            max-width: 800px;
            margin: 0 auto;
        }
        .card {
            background: white;
            border-radius: 12px;
            padding: 24px;
            margin-bottom: 20px;
            box-shadow: 0 4px 6px rgba(0,0,0,0.1);
        }
        h1 { color: #667eea; margin-bottom: 8px; }
        h2 { color: #333; margin-bottom: 16px; font-size: 1.2em; }
        .status { color: #10b981; font-weight: bold; }
        .info { display: grid; grid-template-columns: 1fr 1fr; gap: 12px; margin: 16px 0; }
        .info-item { padding: 12px; background: #f9fafb; border-radius: 8px; }
        .info-label { font-size: 0.85em; color: #6b7280; }
        .info-value { font-size: 1.1em; font-weight: 600; color: #111827; margin-top: 4px; }
        .app-grid { display: grid; grid-template-columns: repeat(auto-fill, minmax(200px, 1fr)); gap: 12px; margin-top: 16px; }
        .app-card {
            padding: 16px;
            background: #f9fafb;
            border-radius: 8px;
            border: 2px solid transparent;
            cursor: pointer;
            transition: all 0.2s;
            position: relative;
        }
        .app-card:hover { border-color: #667eea; transform: translateY(-2px); }
        .app-card.active { border-color: #10b981; background: #ecfdf5; }
        .app-card.loading { 
            opacity: 0.6; 
            pointer-events: none;
        }
        .app-card.loading::after {
            content: '';
            position: absolute;
            top: 50%;
            left: 50%;
            width: 20px;
            height: 20px;
            margin: -10px 0 0 -10px;
            border: 2px solid #667eea;
            border-top-color: transparent;
            border-radius: 50%;
            animation: spin 0.6s linear infinite;
        }
        @keyframes spin {
            to { transform: rotate(360deg); }
        }
        .app-name { font-weight: 600; color: #111827; margin-bottom: 4px; }
        .app-desc { font-size: 0.85em; color: #6b7280; }
        .btn {
            display: inline-block;
            padding: 10px 20px;
            background: #667eea;
            color: white;
            border: none;
            border-radius: 8px;
            cursor: pointer;
            font-size: 0.95em;
            margin: 4px;
            transition: background 0.2s;
        }
        .btn:hover { background: #5568d3; }
        .btn:disabled {
            opacity: 0.6;
            cursor: not-allowed;
        }
        .btn-danger { background: #ef4444; }
        .btn-danger:hover { background: #dc2626; }
        .loading { text-align: center; padding: 20px; color: #6b7280; }
        .spinner {
            display: inline-block;
            width: 16px;
            height: 16px;
            border: 2px solid white;
            border-top-color: transparent;
            border-radius: 50%;
            animation: spin 0.6s linear infinite;
            margin-left: 8px;
            vertical-align: middle;
        }
    </style>
</head>
<body>
    <div class="container">
        <div class="card">
            <h1>🎨 Doki OS Dashboard</h1>
            <p class="status" id="status">Loading...</p>
        </div>

        <div class="card">
            <h2>📊 System Status</h2>
            <div class="info">
                <div class="info-item">
                    <div class="info-label">Current App</div>
                    <div class="info-value" id="currentApp">-</div>
                </div>
                <div class="info-item">
                    <div class="info-label">Uptime</div>
                    <div class="info-value" id="uptime">-</div>
                </div>
                <div class="info-item">
                    <div class="info-label">Heap Memory</div>
                    <div class="info-value" id="heap">-</div>
                </div>
                <div class="info-item">
                    <div class="info-label">PSRAM</div>
                    <div class="info-value" id="psram">-</div>
                </div>
            </div>
        </div>

        <div class="card">
            <h2>📱 Available Apps</h2>
            <div id="appList" class="app-grid">
                <div class="loading">Loading apps...</div>
            </div>
            <div style="margin-top: 16px;">
                <button class="btn btn-danger" onclick="unloadApp()" id="unloadBtn">Unload Current App</button>
            </div>
        </div>
    </div>

    <script>
        const API = window.location.origin + '/api';

        async function loadData() {
            try {
                const [status, apps] = await Promise.all([
                    fetch(API + '/system/status').then(r => r.json()),
                    fetch(API + '/app/list').then(r => r.json())
                ]);

                document.getElementById('status').textContent = '✓ Connected';
                document.getElementById('currentApp').textContent = status.current_app || 'None';
                document.getElementById('uptime').textContent = status.uptime_seconds + 's';
                document.getElementById('heap').textContent = 
                    ((status.memory.total_heap - status.memory.free_heap) / 1024).toFixed(1) + 'KB / ' +
                    (status.memory.total_heap / 1024).toFixed(1) + 'KB';
                document.getElementById('psram').textContent = 
                    ((status.memory.total_psram - status.memory.free_psram) / 1024).toFixed(1) + 'KB / ' +
                    (status.memory.total_psram / 1024).toFixed(1) + 'KB';

                const appList = document.getElementById('appList');
                appList.innerHTML = apps.apps.map(app => `
                    <div class="app-card ${app.loaded ? 'active' : ''}" onclick="loadApp('${app.id}')">
                        <div class="app-name">${app.name}</div>
                        <div class="app-desc">${app.description || app.id}</div>
                        ${app.loaded ? '<div style="color: #10b981; font-size: 0.85em; margin-top: 8px;">● Running</div>' : ''}
                    </div>
                `).join('');
            } catch (err) {
                document.getElementById('status').textContent = '✗ Error loading data';
            }
        }

        async function loadApp(id) {
            const card = document.querySelector(`[onclick="loadApp('${id}')"]`);
            if (!card) return;
            
            // Add loading state
            card.classList.add('loading');
            
            try {
                const res = await fetch(API + '/app/load?id=' + id, { method: 'POST' });
                const data = await res.json();
                
                setTimeout(() => {
                    loadData();
                    card.classList.remove('loading');
                }, 500);
            } catch (err) {
                alert('Failed to load app');
                card.classList.remove('loading');
            }
        }

        async function unloadApp() {
            const btn = document.getElementById('unloadBtn');
            const originalText = btn.textContent;
            
            btn.disabled = true;
            btn.innerHTML = 'Unloading<span class="spinner"></span>';
            
            try {
                const res = await fetch(API + '/app/unload', { method: 'POST' });
                const data = await res.json();
                
                setTimeout(() => {
                    loadData();
                    btn.disabled = false;
                    btn.textContent = originalText;
                }, 500);
            } catch (err) {
                alert('Failed to unload app');
                btn.disabled = false;
                btn.textContent = originalText;
            }
        }

        loadData();
        setInterval(loadData, 2000);
    </script>
</body>
</html>
)rawliteral";
    
    request->send(200, "text/html", html);
}

void HttpServer::handleNotFound(AsyncWebServerRequest* request) {
    sendErrorResponse(request, 404, "Endpoint not found");
}

// ========================================
// Helper Methods
// ========================================

void HttpServer::sendJsonResponse(AsyncWebServerRequest* request, 
                                  int statusCode, 
                                  const JsonDocument& doc) {
    String output;
    serializeJson(doc, output);
    request->send(statusCode, "application/json", output);
}

void HttpServer::sendErrorResponse(AsyncWebServerRequest* request, 
                                   int statusCode, 
                                   const char* message) {
    JsonDocument doc;
    doc["status"] = "error";
    doc["message"] = message;
    sendJsonResponse(request, statusCode, doc);
}

} // namespace Doki