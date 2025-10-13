/**
 * @file main.cpp
 * @brief Dual Display System with HTTP Control
 * 
 * Supports independent app control per display via web dashboard
 */

#include <Arduino.h>
#include <WiFi.h>
#include <SPI.h>
#include <lvgl.h>
#include <ESPAsyncWebServer.h>
#include <ArduinoJson.h>
#include "doki/weather_service.h"

// Import apps
#include "apps/clock_app/clock_app.h"
#include "apps/weather_app/weather_app.h"
#include "apps/sysinfo_app/sysinfo_app.h"
#include "apps/hello_app/hello_app.h"
#include "apps/goodbye_app/goodbye_app.h"
#include "apps/blank_app/blank_app.h"

// ========================================
// Configuration
// ========================================
const char* WIFI_SSID = "Abhi";
const char* WIFI_PASSWORD = "";
const char* WEATHER_API_KEY = "3183db8ec2fe4abfa2c133226251310";

// Display Hardware
#define DISP0_CS 33
#define DISP0_DC 15
#define DISP0_RST 16
#define DISP1_CS 34
#define DISP1_DC 17
#define DISP1_RST 18
#define TFT_MOSI 37
#define TFT_SCLK 36
#define TFT_WIDTH 240
#define TFT_HEIGHT 320

// ST7789 Commands
#define ST7789_SWRESET 0x01
#define ST7789_SLPOUT 0x11
#define ST7789_COLMOD 0x3A
#define ST7789_MADCTL 0x36
#define ST7789_NORON 0x13
#define ST7789_DISPON 0x29
#define ST7789_CASET 0x2A
#define ST7789_RASET 0x2B
#define ST7789_RAMWR 0x2C

// ========================================
// Display Structure
// ========================================
struct Display {
    uint8_t id;
    uint8_t cs_pin;
    uint8_t dc_pin;
    uint8_t rst_pin;
    
    lv_disp_draw_buf_t draw_buf;
    lv_color_t* buf1;
    lv_color_t* buf2;
    lv_disp_drv_t disp_drv;
    lv_disp_t* disp;
    
    Doki::DokiApp* app;
    String appId;
    uint32_t appStartTime;
};

Display displays[2];
SPIClass spi(HSPI);
AsyncWebServer server(80);

// ========================================
// ST7789 Functions
// ========================================
void writeCommand(uint8_t cs, uint8_t dc, uint8_t cmd) {
    digitalWrite(dc, LOW);
    digitalWrite(cs, LOW);
    spi.transfer(cmd);
    digitalWrite(cs, HIGH);
}

void writeData(uint8_t cs, uint8_t dc, uint8_t data) {
    digitalWrite(dc, HIGH);
    digitalWrite(cs, LOW);
    spi.transfer(data);
    digitalWrite(cs, HIGH);
}

void writeData16(uint8_t cs, uint8_t dc, uint16_t data) {
    digitalWrite(dc, HIGH);
    digitalWrite(cs, LOW);
    spi.transfer16(data);
    digitalWrite(cs, HIGH);
}

void setAddrWindow(uint8_t cs, uint8_t dc, uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1) {
    writeCommand(cs, dc, ST7789_CASET);
    writeData16(cs, dc, x0);
    writeData16(cs, dc, x1);
    writeCommand(cs, dc, ST7789_RASET);
    writeData16(cs, dc, y0);
    writeData16(cs, dc, y1);
    writeCommand(cs, dc, ST7789_RAMWR);
}

void initDisplayHardware(uint8_t cs, uint8_t dc, uint8_t rst) {
    pinMode(cs, OUTPUT);
    pinMode(dc, OUTPUT);
    pinMode(rst, OUTPUT);
    digitalWrite(cs, HIGH);
    digitalWrite(dc, HIGH);
    
    digitalWrite(rst, HIGH);
    delay(10);
    digitalWrite(rst, LOW);
    delay(20);
    digitalWrite(rst, HIGH);
    delay(150);
    
    writeCommand(cs, dc, ST7789_SWRESET);
    delay(150);
    writeCommand(cs, dc, ST7789_SLPOUT);
    delay(120);
    writeCommand(cs, dc, ST7789_COLMOD);
    writeData(cs, dc, 0x55);
    delay(10);
    writeCommand(cs, dc, ST7789_MADCTL);
    writeData(cs, dc, 0x00);
    writeCommand(cs, dc, ST7789_NORON);
    delay(10);
    writeCommand(cs, dc, ST7789_DISPON);
    delay(100);
}

void lvgl_flush_display0(lv_disp_drv_t* disp, const lv_area_t* area, lv_color_t* color_p) {
    uint32_t w = (area->x2 - area->x1 + 1);
    uint32_t h = (area->y2 - area->y1 + 1);
    setAddrWindow(DISP0_CS, DISP0_DC, area->x1, area->y1, area->x2, area->y2);
    digitalWrite(DISP0_DC, HIGH);
    digitalWrite(DISP0_CS, LOW);
    for (uint32_t i = 0; i < w * h; i++) {
        spi.transfer16(color_p[i].full);
    }
    digitalWrite(DISP0_CS, HIGH);
    lv_disp_flush_ready(disp);
}

void lvgl_flush_display1(lv_disp_drv_t* disp, const lv_area_t* area, lv_color_t* color_p) {
    uint32_t w = (area->x2 - area->x1 + 1);
    uint32_t h = (area->y2 - area->y1 + 1);
    setAddrWindow(DISP1_CS, DISP1_DC, area->x1, area->y1, area->x2, area->y2);
    digitalWrite(DISP1_DC, HIGH);
    digitalWrite(DISP1_CS, LOW);
    for (uint32_t i = 0; i < w * h; i++) {
        spi.transfer16(color_p[i].full);
    }
    digitalWrite(DISP1_CS, HIGH);
    lv_disp_flush_ready(disp);
}

bool initDisplay(uint8_t id, uint8_t cs, uint8_t dc, uint8_t rst) {
    Display* d = &displays[id];
    d->id = id;
    d->cs_pin = cs;
    d->dc_pin = dc;
    d->rst_pin = rst;
    d->app = nullptr;
    d->appId = "";
    d->appStartTime = 0;
    
    initDisplayHardware(cs, dc, rst);
    
    size_t buf_size = TFT_WIDTH * 40;
    d->buf1 = (lv_color_t*)heap_caps_malloc(buf_size * sizeof(lv_color_t), MALLOC_CAP_SPIRAM);
    d->buf2 = (lv_color_t*)heap_caps_malloc(buf_size * sizeof(lv_color_t), MALLOC_CAP_SPIRAM);
    
    if (!d->buf1 || !d->buf2) return false;
    
    lv_disp_draw_buf_init(&d->draw_buf, d->buf1, d->buf2, buf_size);
    lv_disp_drv_init(&d->disp_drv);
    d->disp_drv.hor_res = TFT_WIDTH;
    d->disp_drv.ver_res = TFT_HEIGHT;
    d->disp_drv.flush_cb = (id == 0) ? lvgl_flush_display0 : lvgl_flush_display1;
    d->disp_drv.draw_buf = &d->draw_buf;
    d->disp = lv_disp_drv_register(&d->disp_drv);
    
    return d->disp != nullptr;
}

// ========================================
// App Management
// ========================================
Doki::DokiApp* createApp(const String& appId) {
    if (appId == "clock") return new ClockApp();
    if (appId == "weather") return new WeatherApp();
    if (appId == "sysinfo") return new SysInfoApp();
    if (appId == "hello") return new HelloApp();
    if (appId == "goodbye") return new GoodbyeApp();
    if (appId == "blank") return new BlankApp();
    return nullptr;
}

bool loadAppOnDisplay(uint8_t displayId, const String& appId) {
    if (displayId >= 2) return false;
    
    Display* d = &displays[displayId];
    
    // Unload current app
    if (d->app) {
        Serial.printf("[Display %d] Unloading '%s'\n", displayId, d->appId.c_str());
        d->app->onPause();
        d->app->onDestroy();
        delete d->app;
        d->app = nullptr;
        d->appId = "";
    }
    
    // Set LVGL context
    lv_disp_set_default(d->disp);
    lv_obj_clean(lv_scr_act());
    
    // Create and load new app
    Serial.printf("[Display %d] Loading '%s'\n", displayId, appId.c_str());
    d->app = createApp(appId);
    
    if (!d->app) {
        Serial.printf("[Display %d] Failed to create app '%s'\n", displayId, appId.c_str());
        return false;
    }
    
    d->appId = appId;
    d->appStartTime = millis();
    
    d->app->onCreate();
    d->app->_setState(Doki::AppState::CREATED);
    d->app->onStart();
    d->app->_setState(Doki::AppState::STARTED);
    d->app->_markStarted();
    
    Serial.printf("[Display %d] ✓ '%s' loaded\n", displayId, appId.c_str());
    return true;
}

// ========================================
// HTTP API Handlers
// ========================================
void handleLoadAppOnDisplay(AsyncWebServerRequest* request) {
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
    
    uint32_t startTime = millis();
    bool success = loadAppOnDisplay(displayId, appId);
    uint32_t loadTime = millis() - startTime;
    
    if (success) {
        JsonDocument doc;
        doc["status"] = "success";
        doc["display"] = displayId;
        doc["app"] = appId;
        doc["load_time_ms"] = loadTime;
        String response;
        serializeJson(doc, response);
        request->send(200, "application/json", response);
    } else {
        request->send(500, "application/json", "{\"error\":\"Failed to load app\"}");
    }
}

void handleDisplaysStatus(AsyncWebServerRequest* request) {
    JsonDocument doc;
    JsonArray disps = doc["displays"].to<JsonArray>();
    
    for (int i = 0; i < 2; i++) {
        JsonObject d = disps.add<JsonObject>();
        d["id"] = i;
        d["app"] = displays[i].appId;
        d["uptime_ms"] = displays[i].app ? (millis() - displays[i].appStartTime) : 0;
        d["running"] = displays[i].app != nullptr;
    }
    
    doc["total_displays"] = 2;
    
    String response;
    serializeJson(doc, response);
    request->send(200, "application/json", response);
}

void handleSystemStatus(AsyncWebServerRequest* request) {
    JsonDocument doc;
    doc["doki_version"] = "0.1.0";
    doc["uptime_seconds"] = millis() / 1000;
    
    JsonObject wifi = doc["wifi"].to<JsonObject>();
    wifi["connected"] = WiFi.status() == WL_CONNECTED;
    wifi["ssid"] = WiFi.SSID();
    wifi["rssi"] = WiFi.RSSI();
    wifi["ip"] = WiFi.localIP().toString();
    
    JsonObject memory = doc["memory"].to<JsonObject>();
    memory["free_heap"] = ESP.getFreeHeap();
    memory["total_heap"] = ESP.getHeapSize();
    memory["free_psram"] = ESP.getFreePsram();
    memory["total_psram"] = ESP.getPsramSize();
    
    String response;
    serializeJson(doc, response);
    request->send(200, "application/json", response);
}

void handleDashboard(AsyncWebServerRequest* request) {
    String html = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Doki OS - Dual Display Dashboard</title>
    <style>
        * { margin: 0; padding: 0; box-sizing: border-box; }
        body {
            font-family: -apple-system, BlinkMacSystemFont, 'Segoe UI', sans-serif;
            background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
            min-height: 100vh;
            padding: 20px;
        }
        .container { max-width: 1200px; margin: 0 auto; }
        .card {
            background: white;
            border-radius: 12px;
            padding: 24px;
            margin-bottom: 20px;
            box-shadow: 0 4px 6px rgba(0,0,0,0.1);
        }
        h1 { color: #667eea; margin-bottom: 8px; }
        h2 { color: #333; margin-bottom: 16px; font-size: 1.2em; }
        .displays-grid {
            display: grid;
            grid-template-columns: repeat(auto-fit, minmax(400px, 1fr));
            gap: 20px;
        }
        .display-card {
            background: #f9fafb;
            border-radius: 12px;
            padding: 20px;
            border: 3px solid #e5e7eb;
        }
        .display-card.active { border-color: #10b981; background: #ecfdf5; }
        .display-header {
            display: flex;
            justify-content: space-between;
            align-items: center;
            margin-bottom: 16px;
        }
        .display-title {
            font-size: 1.3em;
            font-weight: 600;
            color: #111827;
        }
        .display-status {
            padding: 4px 12px;
            border-radius: 12px;
            font-size: 0.85em;
            font-weight: 600;
        }
        .status-active { background: #10b981; color: white; }
        .status-empty { background: #6b7280; color: white; }
        .current-app {
            background: white;
            padding: 16px;
            border-radius: 8px;
            margin-bottom: 16px;
        }
        .app-name { font-size: 1.1em; font-weight: 600; color: #667eea; }
        .app-uptime { color: #6b7280; font-size: 0.9em; margin-top: 4px; }
        .app-buttons {
            display: grid;
            grid-template-columns: repeat(2, 1fr);
            gap: 8px;
        }
        .btn {
            padding: 10px;
            background: #667eea;
            color: white;
            border: none;
            border-radius: 8px;
            cursor: pointer;
            font-size: 0.9em;
            transition: all 0.2s;
        }
        .btn:hover { background: #5568d3; transform: translateY(-1px); }
        .btn:disabled { opacity: 0.5; cursor: not-allowed; }
        .btn.loading {
            opacity: 0.6;
            pointer-events: none;
        }
        .spinner {
            display: inline-block;
            width: 14px;
            height: 14px;
            border: 2px solid white;
            border-top-color: transparent;
            border-radius: 50%;
            animation: spin 0.6s linear infinite;
            margin-left: 6px;
            vertical-align: middle;
        }
        @keyframes spin {
            to { transform: rotate(360deg); }
        }
        .info-grid {
            display: grid;
            grid-template-columns: repeat(3, 1fr);
            gap: 12px;
            margin: 16px 0;
        }
        .info-item {
            padding: 12px;
            background: #f9fafb;
            border-radius: 8px;
        }
        .info-label { font-size: 0.85em; color: #6b7280; }
        .info-value { font-size: 1.1em; font-weight: 600; color: #111827; margin-top: 4px; }
    </style>
</head>
<body>
    <div class="container">
        <div class="card">
            <h1>🎨 Doki OS - Dual Display Dashboard</h1>
            <p style="color: #10b981; font-weight: bold;">✓ Connected</p>
        </div>

        <div class="card">
            <h2>📊 System Status</h2>
            <div class="info-grid">
                <div class="info-item">
                    <div class="info-label">Uptime</div>
                    <div class="info-value" id="uptime">-</div>
                </div>
                <div class="info-item">
                    <div class="info-label">Heap Memory</div>
                    <div class="info-value" id="heap">-</div>
                </div>
                <div class="info-item">
                    <div class="info-label">WiFi</div>
                    <div class="info-value" id="wifi">-</div>
                </div>
            </div>
        </div>

        <div class="card">
            <h2>🖥️ Displays</h2>
            <div class="displays-grid" id="displaysGrid">
                <div>Loading...</div>
            </div>
        </div>
    </div>

    <script>
        const API = window.location.origin + '/api';
        const apps = [
            { id: 'clock', name: '⏰ Clock' },
            { id: 'weather', name: '🌤️ Weather' },
            { id: 'sysinfo', name: '📊 System' },
            { id: 'blank', name: '🌟 Screensaver' },
            { id: 'hello', name: '👋 Hello' },
            { id: 'goodbye', name: '👋 Goodbye' }
        ];

        async function loadData() {
            try {
                const [status, displays] = await Promise.all([
                    fetch(API + '/system/status').then(r => r.json()),
                    fetch(API + '/displays/status').then(r => r.json())
                ]);

                // Update system info
                document.getElementById('uptime').textContent = status.uptime_seconds + 's';
                document.getElementById('heap').textContent = 
                    Math.round((status.memory.total_heap - status.memory.free_heap) / 1024) + ' KB';
                document.getElementById('wifi').textContent = status.wifi.rssi + ' dBm';

                // Update displays
                const grid = document.getElementById('displaysGrid');
                grid.innerHTML = displays.displays.map(d => `
                    <div class="display-card ${d.running ? 'active' : ''}">
                        <div class="display-header">
                            <div class="display-title">Display ${d.id}</div>
                            <span class="display-status ${d.running ? 'status-active' : 'status-empty'}">
                                ${d.running ? 'ACTIVE' : 'EMPTY'}
                            </span>
                        </div>
                        ${d.running ? `
                            <div class="current-app">
                                <div class="app-name">📱 ${d.app}</div>
                                <div class="app-uptime">Uptime: ${Math.floor(d.uptime_ms / 1000)}s</div>
                            </div>
                        ` : '<div style="padding: 20px; text-align: center; color: #6b7280;">No app loaded</div>'}
                        <div class="app-buttons">
                            ${apps.map(app => `
                                <button class="btn" onclick="loadApp(${d.id}, '${app.id}')" 
                                        id="btn-${d.id}-${app.id}">
                                    ${app.name}
                                </button>
                            `).join('')}
                        </div>
                    </div>
                `).join('');
            } catch (err) {
                console.error('Error loading data:', err);
            }
        }

        async function loadApp(displayId, appId) {
            const btn = document.getElementById(`btn-${displayId}-${appId}`);
            if (!btn) return;

            const originalText = btn.textContent;
            btn.classList.add('loading');
            btn.innerHTML = originalText + '<span class="spinner"></span>';

            try {
                const res = await fetch(
                    `${API}/display/load?display=${displayId}&app=${appId}`,
                    { method: 'POST' }
                );
                const data = await res.json();

                if (data.status === 'success') {
                    setTimeout(() => {
                        loadData();
                        btn.classList.remove('loading');
                        btn.textContent = originalText;
                    }, 500);
                } else {
                    alert('Failed to load app: ' + (data.error || 'Unknown error'));
                    btn.classList.remove('loading');
                    btn.textContent = originalText;
                }
            } catch (err) {
                alert('Failed to load app');
                btn.classList.remove('loading');
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

// ========================================
// Setup
// ========================================
void setup() {
    Serial.begin(115200);
    delay(1000);
    
    Serial.println("\n╔═══════════════════════════════════════════════════╗");
    Serial.println("║    🎨 Doki OS - Dual Display + HTTP Control      ║");
    Serial.println("╚═══════════════════════════════════════════════════╝\n");
    
    // Initialize SPI
    spi.begin(TFT_SCLK, -1, TFT_MOSI);
    spi.setFrequency(40000000);
    
    // Initialize LVGL
    lv_init();
    
    // Initialize displays
    if (!initDisplay(0, DISP0_CS, DISP0_DC, DISP0_RST) ||
        !initDisplay(1, DISP1_CS, DISP1_DC, DISP1_RST)) {
        Serial.println("FATAL: Display init failed!");
        while(1) delay(1000);
    }
    
    // Connect WiFi
    WiFi.mode(WIFI_STA);
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    Serial.printf("\n✓ WiFi: %s\n", WiFi.localIP().toString().c_str());
    
    // Initialize services
    Doki::WeatherService::init(WEATHER_API_KEY);
    
    // Setup HTTP server
    server.on("/api/display/load", HTTP_POST, handleLoadAppOnDisplay);
    server.on("/api/displays/status", HTTP_GET, handleDisplaysStatus);
    server.on("/api/system/status", HTTP_GET, handleSystemStatus);
    server.on("/", HTTP_GET, handleDashboard);
    server.on("/dashboard", HTTP_GET, handleDashboard);
    
    DefaultHeaders::Instance().addHeader("Access-Control-Allow-Origin", "*");
    server.begin();
    
    Serial.printf("\n📱 Dashboard: http://%s/dashboard\n\n", WiFi.localIP().toString().c_str());
    
    // Load default apps
    loadAppOnDisplay(0, "clock");
    loadAppOnDisplay(1, "weather");
    
    Serial.println("✓ System ready! Open dashboard to control displays.\n");
}

void loop() {
    static uint32_t lastLvglUpdate = 0;
    uint32_t now = millis();
    
    // Update LVGL at ~30ms intervals (33 FPS)
    if (now - lastLvglUpdate >= 30) {
        lv_timer_handler();
        lastLvglUpdate = now;
    }
    
    // Update each app independently
    for (int i = 0; i < 2; i++) {
        if (displays[i].app) {
            lv_disp_set_default(displays[i].disp);
            displays[i].app->onUpdate();
        }
    }
    
    // Much smaller delay - let apps control their own timing
    delay(1);
}