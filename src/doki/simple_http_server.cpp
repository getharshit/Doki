/**
 * @file simple_http_server.cpp
 * @brief Implementation of Simple HTTP Server
 */

#include "doki/simple_http_server.h"
#include "doki/media_service.h"
#include "doki/app_manager.h"
#include "doki/filesystem_manager.h"
#include <WiFi.h>

namespace Doki {

// Static member initialization
AsyncWebServer* SimpleHttpServer::_server = nullptr;
bool SimpleHttpServer::_running = false;
bool (*SimpleHttpServer::_loadAppCallback)(uint8_t, const String&) = nullptr;
void (*SimpleHttpServer::_statusCallback)(uint8_t, String&, uint32_t&) = nullptr;

// Upload state
uint8_t SimpleHttpServer::_uploadDisplayId = 0;
String SimpleHttpServer::_uploadMediaType = "";
std::vector<uint8_t> SimpleHttpServer::_uploadBuffer;

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

    // API: Get media info
    _server->on("/api/media/info", HTTP_GET, handleMediaInfo);

    // API: Delete media
    _server->on("/api/media/delete", HTTP_DELETE, handleMediaDelete);

    // API: Upload media (with body handler for multipart uploads)
    _server->on("/api/media/upload", HTTP_POST,
                [](AsyncWebServerRequest* request) {
                    // This is called after upload is complete
                    request->send(200, "application/json", "{\"success\":true,\"message\":\"Upload complete\"}");
                },
                handleMediaUpload);

    // API: Upload custom JavaScript code
    _server->on("/api/upload-js", HTTP_POST, handleUploadJS);

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

    // Get apps from AppManager registry
    auto registeredApps = AppManager::getRegisteredApps();
    for (const auto& app : registeredApps) {
        JsonObject appObj = apps.add<JsonObject>();
        appObj["id"] = app.id;
        appObj["name"] = app.name;
        appObj["description"] = app.description;
    }

    String response;
    serializeJson(doc, response);
    request->send(200, "application/json", response);
}

void SimpleHttpServer::handleLoadApp(AsyncWebServerRequest* request) {
    if (!request->hasParam("display") || !request->hasParam("app")) {
        request->send(400, "application/json", "{\"error\":\"Missing display or app parameter\"}");
        return;
    }

    uint8_t displayId = request->getParam("display")->value().toInt();
    String appId = request->getParam("app")->value();

    // Validate display ID
    if (displayId >= AppManager::getNumDisplays()) {
        request->send(400, "application/json", "{\"error\":\"Invalid display ID\"}");
        return;
    }

    // Load app using AppManager
    bool success = AppManager::loadApp(displayId, appId.c_str());

    if (success) {
        request->send(200, "application/json", "{\"success\":true}");
    } else {
        request->send(500, "application/json", "{\"error\":\"Failed to load app\"}");
    }
}

void SimpleHttpServer::handleGetStatus(AsyncWebServerRequest* request) {
    JsonDocument doc;
    JsonArray disps = doc["displays"].to<JsonArray>();

    uint8_t numDisplays = AppManager::getNumDisplays();
    for (uint8_t i = 0; i < numDisplays; i++) {
        const char* appId = AppManager::getAppId(i);
        uint32_t uptime = AppManager::getAppUptime(i) / 1000; // Convert ms to seconds

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
            position: relative;
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
        .message.info { background: #dbeafe; color: #1e40af; display: block; }
        .upload-section {
            margin-top: 32px;
            padding: 24px;
            background: #f9fafb;
            border-radius: 12px;
        }
        .upload-grid {
            display: grid;
            grid-template-columns: repeat(auto-fit, minmax(350px, 1fr));
            gap: 24px;
            margin-top: 16px;
        }
        .upload-card {
            background: white;
            padding: 20px;
            border-radius: 8px;
            border: 2px solid #e5e7eb;
        }
        .upload-card h3 { color: #667eea; margin-bottom: 16px; }
        .file-input-wrapper {
            margin: 16px 0;
        }
        .file-input-wrapper label {
            display: block;
            margin-bottom: 8px;
            font-weight: 600;
            color: #374151;
        }
        .file-input {
            display: block;
            width: 100%;
            padding: 8px;
            border: 2px dashed #d1d5db;
            border-radius: 6px;
            cursor: pointer;
        }
        .upload-btn {
            width: 100%;
            padding: 12px;
            background: #10b981;
            color: white;
            border: none;
            border-radius: 6px;
            font-weight: 600;
            cursor: pointer;
            margin-top: 12px;
        }
        .upload-btn:hover { background: #059669; }
        .upload-btn:disabled { background: #9ca3af; cursor: not-allowed; }
        .media-info {
            margin-top: 12px;
            padding: 12px;
            background: #f3f4f6;
            border-radius: 6px;
            font-size: 0.9em;
        }
        .delete-btn {
            width: 100%;
            padding: 10px;
            background: #ef4444;
            color: white;
            border: none;
            border-radius: 6px;
            font-weight: 600;
            cursor: pointer;
            margin-top: 8px;
        }
        .delete-btn:hover { background: #dc2626; }
        progress {
            width: 100%;
            height: 8px;
            margin-top: 8px;
        }
        .loading-spinner {
            display: inline-block;
            width: 16px;
            height: 16px;
            border: 3px solid rgba(255,255,255,.3);
            border-radius: 50%;
            border-top-color: white;
            animation: spin 0.8s linear infinite;
            margin-left: 8px;
            vertical-align: middle;
        }
        @keyframes spin {
            to { transform: rotate(360deg); }
        }
        .display-card.loading {
            opacity: 0.6;
            pointer-events: none;
        }
        .display-card.loading::after {
            content: 'Loading...';
            position: absolute;
            top: 50%;
            left: 50%;
            transform: translate(-50%, -50%);
            background: rgba(102, 126, 234, 0.95);
            color: white;
            padding: 12px 24px;
            border-radius: 8px;
            font-weight: 600;
        }
        .app-btn:disabled {
            opacity: 0.5;
            cursor: not-allowed;
        }
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
        <div class="upload-section">
            <h2>📷 Media Upload</h2>
            <p style="color: #6b7280; margin-bottom: 16px;">Upload images (PNG/JPEG) or GIFs for each display (max 1MB, auto-resized to 240x320)</p>
            <div class="upload-grid">
                <div class="upload-card">
                    <h3>Display 0</h3>
                    <div class="file-input-wrapper">
                        <label>Image (PNG/JPEG):</label>
                        <input type="file" id="d0-image-file" class="file-input" accept=".png,.jpg,.jpeg">
                        <button onclick="uploadMedia(0, 'image')" class="upload-btn" id="d0-image-btn">Upload Image</button>
                        <progress id="d0-image-progress" value="0" max="100" style="display:none"></progress>
                    </div>
                    <div class="file-input-wrapper">
                        <label>Animated GIF:</label>
                        <input type="file" id="d0-gif-file" class="file-input" accept=".gif">
                        <button onclick="uploadMedia(0, 'gif')" class="upload-btn" id="d0-gif-btn">Upload GIF</button>
                        <progress id="d0-gif-progress" value="0" max="100" style="display:none"></progress>
                    </div>
                    <div id="d0-media-info" class="media-info"></div>
                </div>
                <div class="upload-card">
                    <h3>Display 1</h3>
                    <div class="file-input-wrapper">
                        <label>Image (PNG/JPEG):</label>
                        <input type="file" id="d1-image-file" class="file-input" accept=".png,.jpg,.jpeg">
                        <button onclick="uploadMedia(1, 'image')" class="upload-btn" id="d1-image-btn">Upload Image</button>
                        <progress id="d1-image-progress" value="0" max="100" style="display:none"></progress>
                    </div>
                    <div class="file-input-wrapper">
                        <label>Animated GIF:</label>
                        <input type="file" id="d1-gif-file" class="file-input" accept=".gif">
                        <button onclick="uploadMedia(1, 'gif')" class="upload-btn" id="d1-gif-btn">Upload GIF</button>
                        <progress id="d1-gif-progress" value="0" max="100" style="display:none"></progress>
                    </div>
                    <div id="d1-media-info" class="media-info"></div>
                </div>
            </div>
        </div>
        <div class="upload-section">
            <h2>⚡ Custom JavaScript</h2>
            <p style="color: #6b7280; margin-bottom: 8px;">Write custom JavaScript code and run it on your displays. Each display has its own custom app.</p>

            <div style="background: white; padding: 20px; border-radius: 8px; margin-bottom: 16px;">
                <div style="margin-bottom: 12px; padding: 10px; background: #EFF6FF; border-left: 3px solid #3B82F6; border-radius: 4px; font-size: 14px;">
                    <strong style="color: #1E40AF;">💡 How it works:</strong>
                    <span style="color: #1E3A8A;"> Select a display above, load "Custom JS" app, then upload your code here. Each display has its own custom code.</span>
                </div>

                <div style="display: flex; justify-content: space-between; align-items: center; margin-bottom: 16px;">
                    <div>
                        <label style="font-weight: 600; margin-right: 12px;">Upload Code to Display:</label>
                        <select id="js-display-select" style="padding: 8px; border-radius: 6px; border: 2px solid #d1d5db; font-size: 14px;">
                            <option value="0">Display 0</option>
                            <option value="1">Display 1</option>
                        </select>
                    </div>
                    <div style="color: #6b7280; font-size: 14px;">
                        <span id="js-char-count">0</span> / 16384 bytes
                    </div>
                </div>

                <div style="margin-bottom: 12px;">
                    <textarea id="js-code-editor" style="width: 100%; height: 300px; font-family: 'Courier New', monospace; font-size: 13px; padding: 12px; border: 2px solid #d1d5db; border-radius: 6px; resize: vertical;" placeholder="Write your JavaScript code here...">// Doki OS JavaScript App - Display-only API
// Full API Reference:
//
// UI: createLabel(t,x,y) -> ID, updateLabel(id,t),
//     setLabelColor(id,hex), setLabelSize(id,size),
//     clearScreen(), setBackgroundColor(hex)
//
// Draw: drawRectangle(x,y,w,h,color), drawCircle(x,y,r,color)
//
// Advanced: createScrollingLabel(t,x,y,w) -> ID,
//           setTextAlign(id,0|1|2)
//
// Screen: getWidth(), getHeight(), getDisplayId()
//
// Time: millis()
//
// HTTP: httpGet(url) -> text|null
//
// JSON: JSON.parse(str), JSON.stringify(obj)
//
// State: saveState(k,v), loadState(k)

var clockLabel;

function onCreate() {
    log("Custom app started on display " + getDisplayId());
    clearScreen();
    setBackgroundColor(0x0f172a);

    // Title
    var titleId = createLabel("Live Clock", 70, 40);
    setLabelColor(titleId, 0x60a5fa);
    setLabelSize(titleId, 20);

    // Clock (will update every second)
    clockLabel = createLabel("00:00:00", 60, 100);
    setLabelSize(clockLabel, 24);
    setLabelColor(clockLabel, 0x10b981);

    // Decorative elements
    drawCircle(120, 160, 80, 0x1e3a8a);
    drawRectangle(40, 220, 160, 60, 0x312e81);

    var infoId = createLabel("Uptime", 90, 235);
    setLabelColor(infoId, 0x94a3b8);
}

var lastUpdate = 0;

function onUpdate() {
    // Update clock every second
    var now = millis();
    if (now - lastUpdate > 1000) {
        var seconds = Math.floor((now / 1000) % 60);
        var minutes = Math.floor((now / 60000) % 60);
        var hours = Math.floor((now / 3600000) % 24);

        var timeStr =
            (hours < 10 ? "0" : "") + hours + ":" +
            (minutes < 10 ? "0" : "") + minutes + ":" +
            (seconds < 10 ? "0" : "") + seconds;

        updateLabel(clockLabel, timeStr);
        lastUpdate = now;
    }
}

function onDestroy() {
    log("App closed");
}</textarea>
                </div>

                <div style="display: flex; gap: 12px;">
                    <button onclick="uploadAndLoadJS()" style="flex: 1; padding: 14px; background: #10b981; color: white; border: none; border-radius: 6px; font-weight: 600; cursor: pointer; font-size: 15px;">
                        📤 Upload & Load on Display
                    </button>
                    <button onclick="clearJSEditor()" style="padding: 14px 24px; background: #6b7280; color: white; border: none; border-radius: 6px; cursor: pointer;">
                        Clear
                    </button>
                </div>

                <div style="margin-top: 16px; padding: 12px; background: #f3f4f6; border-radius: 6px; font-size: 13px; color: #4b5563;">
                    <strong>💡 Tips:</strong>
                    <ul style="margin: 8px 0 0 20px;">
                        <li>Use <code>onCreate()</code> to initialize your app</li>
                        <li>Use <code>onUpdate()</code> for periodic updates</li>
                        <li>Coordinates: x (0-240), y (0-320)</li>
                        <li>Colors in hex: 0xFF0000 (red), 0x00FF00 (green), 0x0000FF (blue)</li>
                    </ul>
                </div>
            </div>
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
            // Show loading state
            showMessage('Loading ' + appId + ' on Display ' + selectedDisplay + '...', 'info');
            setDisplayLoading(selectedDisplay, true);
            disableAllAppButtons(true);

            const url = '/api/load?display=' + selectedDisplay + '&app=' + appId;
            fetch(url, { method: 'POST' })
                .then(function(res) { return res.json(); })
                .then(function(data) {
                    if (data.success) {
                        showMessage('✓ Loaded ' + appId + ' on Display ' + selectedDisplay, 'success');
                    } else {
                        showMessage('✗ Failed to load app', 'error');
                    }
                })
                .catch(function(err) {
                    showMessage('✗ Error: ' + err.message, 'error');
                })
                .finally(function() {
                    // Remove loading state and refresh status
                    setTimeout(function() {
                        setDisplayLoading(selectedDisplay, false);
                        disableAllAppButtons(false);
                        loadStatus();
                    }, 500);
                });
        }

        function setDisplayLoading(displayId, isLoading) {
            const cards = document.querySelectorAll('.display-card');
            if (cards[displayId]) {
                if (isLoading) {
                    cards[displayId].classList.add('loading');
                } else {
                    cards[displayId].classList.remove('loading');
                }
            }
        }

        function disableAllAppButtons(disabled) {
            const buttons = document.querySelectorAll('.app-btn');
            buttons.forEach(function(btn) {
                btn.disabled = disabled;
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
                        // Handle both old format (string) and new format (object)
                        const appId = typeof app === 'string' ? app : app.id;
                        const appName = typeof app === 'string' ? app : app.name;
                        html += '<button class="app-btn" onclick="loadApp(\'' + appId + '\')">' + appName + '</button>';
                    }
                    appsDiv.innerHTML = html;
                });
        }

        function resizeImage(file, maxWidth, maxHeight, callback) {
            const reader = new FileReader();
            reader.onload = function(e) {
                const img = new Image();
                img.onload = function() {
                    let width = img.width;
                    let height = img.height;
                    if (width > maxWidth || height > maxHeight) {
                        if (width > height) {
                            if (width > maxWidth) {
                                height = height * (maxWidth / width);
                                width = maxWidth;
                            }
                        } else {
                            if (height > maxHeight) {
                                width = width * (maxHeight / height);
                                height = maxHeight;
                            }
                        }
                    }
                    const canvas = document.createElement('canvas');
                    canvas.width = width;
                    canvas.height = height;
                    const ctx = canvas.getContext('2d');
                    ctx.drawImage(img, 0, 0, width, height);
                    canvas.toBlob(function(blob) {
                        callback(blob);
                    }, file.type, 0.9);
                };
                img.src = e.target.result;
            };
            reader.readAsDataURL(file);
        }

        function uploadMedia(displayId, type) {
            const fileInput = document.getElementById('d' + displayId + '-' + type + '-file');
            const btn = document.getElementById('d' + displayId + '-' + type + '-btn');
            const progress = document.getElementById('d' + displayId + '-' + type + '-progress');
            const file = fileInput.files[0];
            if (!file) {
                showMessage('Please select a file first', 'error');
                return;
            }
            if (file.size > 1048576) {
                showMessage('File too large (max 1MB)', 'error');
                return;
            }
            btn.disabled = true;
            btn.textContent = 'Uploading...';
            progress.style.display = 'block';
            progress.value = 0;
            function doUpload(fileToUpload) {
                const formData = new FormData();
                formData.append('display', displayId);
                formData.append('type', type);
                formData.append('file', fileToUpload, file.name);
                const xhr = new XMLHttpRequest();
                xhr.upload.addEventListener('progress', function(e) {
                    if (e.lengthComputable) {
                        progress.value = (e.loaded / e.total) * 100;
                    }
                });
                xhr.addEventListener('load', function() {
                    progress.style.display = 'none';
                    btn.disabled = false;
                    btn.textContent = 'Upload ' + (type === 'image' ? 'Image' : 'GIF');
                    if (xhr.status === 200) {
                        showMessage('Upload successful!', 'success');
                        fileInput.value = '';
                        loadMediaInfo();
                    } else {
                        showMessage('Upload failed', 'error');
                    }
                });
                xhr.addEventListener('error', function() {
                    progress.style.display = 'none';
                    btn.disabled = false;
                    btn.textContent = 'Upload ' + (type === 'image' ? 'Image' : 'GIF');
                    showMessage('Upload error', 'error');
                });
                xhr.open('POST', '/api/media/upload');
                xhr.send(formData);
            }
            if (type === 'image') {
                resizeImage(file, 240, 320, function(blob) {
                    doUpload(blob);
                });
            } else {
                doUpload(file);
            }
        }

        function loadMediaInfo() {
            for (let i = 0; i < 2; i++) {
                fetch('/api/media/info?display=' + i)
                    .then(function(res) { return res.json(); })
                    .then(function(data) {
                        const infoDiv = document.getElementById('d' + i + '-media-info');
                        let html = '';
                        if (data.image && data.image.exists) {
                            const sizeKB = Math.round(data.image.size / 1024);
                            html += '<div><strong>Image:</strong> ' + data.image.type.toUpperCase() + ' (' + sizeKB + ' KB)</div>';
                            html += '<button class="delete-btn" onclick="deleteMedia(' + i + ', \'image\')">Delete Image</button>';
                        }
                        if (data.gif && data.gif.exists) {
                            const sizeKB = Math.round(data.gif.size / 1024);
                            html += '<div><strong>GIF:</strong> ' + sizeKB + ' KB</div>';
                            html += '<button class="delete-btn" onclick="deleteMedia(' + i + ', \'gif\')">Delete GIF</button>';
                        }
                        if (html === '') {
                            html = '<div style="color:#9ca3af">No media uploaded</div>';
                        }
                        infoDiv.innerHTML = html;
                    });
            }
        }

        function deleteMedia(displayId, type) {
            if (!confirm('Delete this ' + type + '?')) return;
            fetch('/api/media/delete?display=' + displayId + '&type=' + type, { method: 'DELETE' })
                .then(function(res) { return res.json(); })
                .then(function(data) {
                    if (data.success) {
                        showMessage('Media deleted', 'success');
                        loadMediaInfo();
                    } else {
                        showMessage('Delete failed', 'error');
                    }
                });
        }

        // Custom JavaScript editor functions
        const jsEditor = document.getElementById('js-code-editor');
        const jsCharCount = document.getElementById('js-char-count');
        const jsDisplaySelect = document.getElementById('js-display-select');

        // Update character count
        jsEditor.addEventListener('input', function() {
            jsCharCount.textContent = jsEditor.value.length;
        });
        jsCharCount.textContent = jsEditor.value.length;

        function uploadAndLoadJS() {
            const code = jsEditor.value;
            const displayId = jsDisplaySelect.value;

            if (code.trim() === '') {
                showMessage('Code is empty', 'error');
                return;
            }

            if (code.length > 16384) {
                showMessage('Code is too large (max 16KB)', 'error');
                return;
            }

            showMessage('Uploading custom JS...', 'info');

            // Upload JS code
            const formData = new FormData();
            formData.append('display', displayId);
            formData.append('code', code);

            fetch('/api/upload-js', {
                method: 'POST',
                body: formData
            })
            .then(function(res) { return res.json(); })
            .then(function(data) {
                if (data.success) {
                    let message = '✓ Code uploaded to Display ' + displayId + ' (' + data.size + ' bytes).';
                    if (data.reloaded) {
                        message += ' App reloaded with new code!';
                    } else {
                        message += ' Load "Custom JS" app to run it.';
                    }
                    showMessage(message, 'success');
                    loadStatus();
                } else {
                    throw new Error(data.error || 'Upload failed');
                }
            })
            .catch(function(err) {
                showMessage('✗ Error: ' + err.message, 'error');
            });
        }

        function clearJSEditor() {
            if (confirm('Clear the editor?')) {
                jsEditor.value = '';
                jsCharCount.textContent = '0';
            }
        }

        loadStatus();
        loadApps();
        loadMediaInfo();
        setInterval(loadStatus, 2000);
    </script>
</body>
</html>
)rawliteral";
}

void SimpleHttpServer::handleMediaInfo(AsyncWebServerRequest* request) {
    if (!request->hasParam("display")) {
        request->send(400, "application/json", "{\"error\":\"Missing display parameter\"}");
        return;
    }

    uint8_t displayId = request->getParam("display")->value().toInt();
    if (displayId > 1) {
        request->send(400, "application/json", "{\"error\":\"Invalid display ID\"}");
        return;
    }

    JsonDocument doc;

    // Check for image (PNG or JPEG)
    MediaInfo imageInfo = MediaService::getMediaInfo(displayId, MediaType::IMAGE_PNG);
    if (!imageInfo.exists) {
        imageInfo = MediaService::getMediaInfo(displayId, MediaType::IMAGE_JPEG);
    }

    if (imageInfo.exists) {
        doc["image"]["exists"] = true;
        doc["image"]["path"] = imageInfo.path;
        doc["image"]["size"] = imageInfo.fileSize;
        doc["image"]["type"] = (imageInfo.type == MediaType::IMAGE_PNG) ? "png" : "jpg";
    } else {
        doc["image"]["exists"] = false;
    }

    // Check for GIF
    MediaInfo gifInfo = MediaService::getMediaInfo(displayId, MediaType::GIF);
    if (gifInfo.exists) {
        doc["gif"]["exists"] = true;
        doc["gif"]["path"] = gifInfo.path;
        doc["gif"]["size"] = gifInfo.fileSize;
    } else {
        doc["gif"]["exists"] = false;
    }

    String response;
    serializeJson(doc, response);
    request->send(200, "application/json", response);
}

void SimpleHttpServer::handleMediaDelete(AsyncWebServerRequest* request) {
    if (!request->hasParam("display") || !request->hasParam("type")) {
        request->send(400, "application/json", "{\"error\":\"Missing display or type parameter\"}");
        return;
    }

    uint8_t displayId = request->getParam("display")->value().toInt();
    String typeStr = request->getParam("type")->value();

    if (displayId > 1) {
        request->send(400, "application/json", "{\"error\":\"Invalid display ID\"}");
        return;
    }

    MediaType type;
    if (typeStr == "image") {
        // Try to delete both PNG and JPEG
        MediaService::deleteMedia(displayId, MediaType::IMAGE_PNG);
        MediaService::deleteMedia(displayId, MediaType::IMAGE_JPEG);
        request->send(200, "application/json", "{\"success\":true,\"message\":\"Image deleted\"}");
        return;
    } else if (typeStr == "gif") {
        type = MediaType::GIF;
    } else {
        request->send(400, "application/json", "{\"error\":\"Invalid type (must be 'image' or 'gif')\"}");
        return;
    }

    if (MediaService::deleteMedia(displayId, type)) {
        request->send(200, "application/json", "{\"success\":true,\"message\":\"Media deleted\"}");
    } else {
        request->send(500, "application/json", "{\"error\":\"Failed to delete media\"}");
    }
}

void SimpleHttpServer::handleUploadJS(AsyncWebServerRequest* request) {
    // Check for required parameters
    if (!request->hasParam("display", true) || !request->hasParam("code", true)) {
        request->send(400, "application/json", "{\"error\":\"Missing display or code parameter\"}");
        return;
    }

    uint8_t displayId = request->getParam("display", true)->value().toInt();
    String code = request->getParam("code", true)->value();

    if (displayId > 1) {
        request->send(400, "application/json", "{\"error\":\"Invalid display ID (must be 0 or 1)\"}");
        return;
    }

    // Check code size (limit to 16KB for safety)
    const size_t MAX_CODE_SIZE = 16 * 1024;
    if (code.length() > MAX_CODE_SIZE) {
        request->send(400, "application/json", "{\"error\":\"Code too large (max 16KB)\"}");
        return;
    }

    if (code.isEmpty()) {
        request->send(400, "application/json", "{\"error\":\"Empty code\"}");
        return;
    }

    // Create /js directory if it doesn't exist
    if (!FilesystemManager::exists("/js")) {
        FilesystemManager::createDir("/js");
    }

    // Save to SPIFFS
    char filepath[32];
    snprintf(filepath, sizeof(filepath), "/js/custom_disp%d.js", displayId);

    // Write file using FilesystemManager API
    bool success = FilesystemManager::writeFile(filepath, (const uint8_t*)code.c_str(), code.length());
    if (!success) {
        request->send(500, "application/json", "{\"error\":\"Failed to write file\"}");
        return;
    }

    size_t written = code.length();

    Serial.printf("[SimpleHTTP] ✓ Saved custom JS for display %d (%d bytes)\n", displayId, written);

    // Check if Custom JS app is currently running on this display
    const char* currentAppId = AppManager::getAppId(displayId);
    bool needsReload = false;
    if (currentAppId && strcmp(currentAppId, "custom") == 0) {
        // Reload the app to pick up the new code
        Serial.printf("[SimpleHTTP] Reloading Custom JS app on display %d\n", displayId);
        if (AppManager::unloadApp(displayId) && AppManager::loadApp(displayId, "custom")) {
            Serial.printf("[SimpleHTTP] ✓ Custom JS app reloaded on display %d\n", displayId);
            needsReload = true;
        } else {
            Serial.printf("[SimpleHTTP] ⚠️ Failed to reload app on display %d\n", displayId);
        }
    }

    // Send success response
    String response = "{\"success\":true,\"size\":" + String(written) + ",\"path\":\"" + filepath + "\",\"reloaded\":" + (needsReload ? "true" : "false") + "}";
    request->send(200, "application/json", response);
}

void SimpleHttpServer::handleMediaUpload(AsyncWebServerRequest* request,
                                          const String& filename,
                                          size_t index,
                                          uint8_t* data,
                                          size_t len,
                                          bool final) {
    // First chunk - initialize upload
    if (index == 0) {
        Serial.printf("[SimpleHTTP] Starting upload: %s\n", filename.c_str());

        // Get display ID and type from request parameters
        if (request->hasParam("display", true)) {
            String displayParam = request->getParam("display", true)->value();
            _uploadDisplayId = displayParam.toInt();
            Serial.printf("[SimpleHTTP] Display parameter: '%s' -> ID: %d\n",
                         displayParam.c_str(), _uploadDisplayId);
        } else {
            Serial.println("[SimpleHTTP] Error: Missing display parameter");
            return;
        }

        if (request->hasParam("type", true)) {
            _uploadMediaType = request->getParam("type", true)->value();
            Serial.printf("[SimpleHTTP] Type parameter: '%s'\n", _uploadMediaType.c_str());
        } else {
            Serial.println("[SimpleHTTP] Error: Missing type parameter");
            return;
        }

        // Clear upload buffer
        _uploadBuffer.clear();
        _uploadBuffer.reserve(MediaService::MAX_FILE_SIZE);
    }

    // Append data to buffer
    for (size_t i = 0; i < len; i++) {
        _uploadBuffer.push_back(data[i]);
    }

    Serial.printf("[SimpleHTTP] Upload progress: %zu bytes\n", _uploadBuffer.size());

    // Final chunk - process upload
    if (final) {
        Serial.printf("[SimpleHTTP] Upload complete: %zu bytes total\n", _uploadBuffer.size());

        // Validate size
        if (_uploadBuffer.size() == 0) {
            Serial.println("[SimpleHTTP] Error: Empty file");
            return;
        }

        if (_uploadBuffer.size() > MediaService::MAX_FILE_SIZE) {
            Serial.printf("[SimpleHTTP] Error: File too large (%zu bytes, max %zu)\n",
                          _uploadBuffer.size(), MediaService::MAX_FILE_SIZE);
            return;
        }

        // Detect media type
        MediaType detectedType = MediaService::detectMediaType(_uploadBuffer.data(), _uploadBuffer.size());

        // Validate type matches request
        MediaType expectedType = MediaType::UNKNOWN;
        if (_uploadMediaType == "image") {
            if (detectedType != MediaType::IMAGE_PNG && detectedType != MediaType::IMAGE_JPEG) {
                Serial.println("[SimpleHTTP] Error: Not a valid image file");
                return;
            }
            expectedType = detectedType; // Use detected type (PNG or JPEG)
        } else if (_uploadMediaType == "gif") {
            if (detectedType != MediaType::GIF) {
                Serial.println("[SimpleHTTP] Error: Not a valid GIF file");
                return;
            }
            expectedType = MediaType::GIF;
        }

        // Save media
        Serial.printf("[SimpleHTTP] Saving media to Display %d, type: %d\n",
                     _uploadDisplayId, (int)expectedType);
        if (MediaService::saveMedia(_uploadDisplayId, _uploadBuffer.data(), _uploadBuffer.size(), expectedType)) {
            Serial.printf("[SimpleHTTP] ✓ Media saved successfully to Display %d\n", _uploadDisplayId);
        } else {
            Serial.printf("[SimpleHTTP] ✗ Failed to save media to Display %d\n", _uploadDisplayId);
        }

        // Clear upload buffer
        _uploadBuffer.clear();
    }
}

} // namespace Doki
