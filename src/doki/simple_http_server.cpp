/**
 * @file simple_http_server.cpp
 * @brief Implementation of Simple HTTP Server
 */

#include "doki/simple_http_server.h"
#include "doki/media_service.h"
#include "doki/media_cache.h"
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

    // API: Upload animation sprite (with body handler for multipart uploads)
    _server->on("/api/animations/upload", HTTP_POST,
                [](AsyncWebServerRequest* request) {
                    // This is called after upload is complete
                    request->send(200, "application/json", "{\"success\":true,\"message\":\"Animation uploaded\"}");
                },
                handleAnimationUpload);

    // API: Upload custom JavaScript code
    _server->on("/api/upload-js", HTTP_POST, handleUploadJS);

    // Enable CORS for all requests
    DefaultHeaders::Instance().addHeader("Access-Control-Allow-Origin", "*");
    DefaultHeaders::Instance().addHeader("Access-Control-Allow-Methods", "GET, POST, PUT, DELETE, OPTIONS");
    DefaultHeaders::Instance().addHeader("Access-Control-Allow-Headers", "Content-Type, Authorization");

    // Handle OPTIONS preflight requests for all endpoints
    _server->onNotFound([](AsyncWebServerRequest *request) {
        if (request->method() == HTTP_OPTIONS) {
            request->send(200);
        } else {
            request->send(404, "text/plain", "Not Found");
        }
    });

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

        // Get display ID and type from URL query parameters (not POST body)
        if (request->hasParam("display", false)) {  // false = query param
            String displayParam = request->getParam("display", false)->value();
            _uploadDisplayId = displayParam.toInt();
            Serial.printf("[SimpleHTTP] Display parameter: '%s' -> ID: %d\n",
                         displayParam.c_str(), _uploadDisplayId);
        } else {
            Serial.println("[SimpleHTTP] Error: Missing display parameter in URL");
            return;
        }

        if (request->hasParam("type", false)) {  // false = query param
            _uploadMediaType = request->getParam("type", false)->value();
            Serial.printf("[SimpleHTTP] Type parameter: '%s'\n", _uploadMediaType.c_str());
        } else {
            Serial.println("[SimpleHTTP] Error: Missing type parameter in URL");
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

    // Debug: Log first 16 bytes of first chunk to detect corruption
    if (index == 0 && len >= 16) {
        Serial.printf("[SimpleHTTP] First 16 bytes received: ");
        for (size_t i = 0; i < 16; i++) {
            Serial.printf("%02X ", data[i]);
        }
        Serial.println();

        // Check magic number for sprite files
        if (_uploadMediaType == "sprite" && len >= 4) {
            uint32_t magic = *((uint32_t*)data);
            Serial.printf("[SimpleHTTP] Magic number: 0x%08X (expected 0x444F4B49 for sprite)\n", magic);
        }
    }

    Serial.printf("[SimpleHTTP] Upload progress: %zu bytes\n", _uploadBuffer.size());

    // Final chunk - process upload
    if (final) {
        Serial.printf("[SimpleHTTP] Upload complete: %zu bytes total\n", _uploadBuffer.size());

        // Debug: Log first 16 bytes of final buffer to verify integrity
        if (_uploadBuffer.size() >= 16) {
            Serial.printf("[SimpleHTTP] First 16 bytes in buffer: ");
            for (size_t i = 0; i < 16; i++) {
                Serial.printf("%02X ", _uploadBuffer[i]);
            }
            Serial.println();

            // Check magic number
            if (_uploadBuffer.size() >= 4) {
                uint32_t magic = *((uint32_t*)_uploadBuffer.data());
                Serial.printf("[SimpleHTTP] Buffer magic number: 0x%08X\n", magic);

                if (_uploadMediaType == "sprite") {
                    const uint32_t SPRITE_MAGIC = 0x444F4B49;  // "DOKI"
                    if (magic != SPRITE_MAGIC) {
                        Serial.printf("[SimpleHTTP] ⚠️ WARNING: Sprite magic mismatch! Got 0x%08X, expected 0x%08X\n",
                                     magic, SPRITE_MAGIC);
                    } else {
                        Serial.println("[SimpleHTTP] ✓ Sprite magic number is correct");
                    }
                }
            }
        }

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
        } else if (_uploadMediaType == "sprite") {
            if (detectedType != MediaType::SPRITE) {
                Serial.println("[SimpleHTTP] Error: Not a valid sprite file");
                return;
            }
            expectedType = MediaType::SPRITE;
        }

        // Load media into PSRAM cache (with optional persistence for small files)
        Serial.printf("[SimpleHTTP] Loading media to cache (Display %d, type: %d)\n",
                     _uploadDisplayId, (int)expectedType);

        // Generate cache ID
        String cacheId = "d" + String(_uploadDisplayId) + "_" + _uploadMediaType;

        if (MediaCache::loadFromMemory(cacheId,
                                      _uploadBuffer.data(),
                                      _uploadBuffer.size(),
                                      expectedType,
                                      _uploadDisplayId,
                                      true)) {  // Try to persist small files
            Serial.printf("[SimpleHTTP] ✓ Media loaded to cache (Display %d)\n", _uploadDisplayId);

            // Reload app to show new media
            String appToLoad;
            if (expectedType == MediaType::SPRITE) {
                appToLoad = "sprite_player";
            } else if (expectedType == MediaType::GIF) {
                appToLoad = "gif";
            } else if (expectedType == MediaType::IMAGE_PNG || expectedType == MediaType::IMAGE_JPEG) {
                appToLoad = "image";
            }

            if (!appToLoad.isEmpty()) {
                AppManager::unloadApp(_uploadDisplayId);
                AppManager::loadApp(_uploadDisplayId, appToLoad.c_str());
                Serial.printf("[SimpleHTTP] ✓ Reloaded app: %s\n", appToLoad.c_str());
            }
        } else {
            Serial.printf("[SimpleHTTP] ✗ Failed to load media to cache (Display %d)\n", _uploadDisplayId);
        }

        // Clear upload buffer
        _uploadBuffer.clear();
    }
}

void SimpleHttpServer::handleAnimationUpload(AsyncWebServerRequest* request,
                                              const String& filename,
                                              size_t index,
                                              uint8_t* data,
                                              size_t len,
                                              bool final) {
    // First chunk - initialize upload
    if (index == 0) {
        Serial.printf("[SimpleHTTP] Starting animation upload: %s\n", filename.c_str());

        // Clear upload buffer
        _uploadBuffer.clear();
        _uploadBuffer.reserve(1024 * 1024);  // Reserve 1MB for animation sprites
    }

    // Append data to buffer
    for (size_t i = 0; i < len; i++) {
        _uploadBuffer.push_back(data[i]);
    }

    Serial.printf("[SimpleHTTP] Animation upload progress: %zu bytes\n", _uploadBuffer.size());

    // Final chunk - process upload
    if (final) {
        Serial.printf("[SimpleHTTP] Animation upload complete: %zu bytes total\n", _uploadBuffer.size());

        // Validate size
        if (_uploadBuffer.size() == 0) {
            Serial.println("[SimpleHTTP] Error: Empty animation file");
            return;
        }

        if (_uploadBuffer.size() > 1024 * 1024) {  // Max 1MB
            Serial.printf("[SimpleHTTP] Error: Animation file too large (%zu bytes, max 1MB)\n",
                          _uploadBuffer.size());
            return;
        }

        // Validate magic number (first 4 bytes should be "DOKI" = 0x444F4B49)
        if (_uploadBuffer.size() < 4) {
            Serial.println("[SimpleHTTP] Error: File too small to be valid animation");
            return;
        }

        uint32_t magic = *((uint32_t*)_uploadBuffer.data());
        const uint32_t SPRITE_MAGIC = 0x444F4B49;  // "DOKI" in ASCII

        if (magic != SPRITE_MAGIC) {
            Serial.printf("[SimpleHTTP] Error: Invalid sprite file (magic: 0x%08X, expected: 0x%08X)\n",
                         magic, SPRITE_MAGIC);
            return;
        }

        Serial.println("[SimpleHTTP] ✓ Valid sprite file detected");

        // Construct file path
        String filepath = "/animations/" + filename;

        // Save to SPIFFS
        if (FilesystemManager::writeFile(filepath.c_str(), _uploadBuffer.data(), _uploadBuffer.size())) {
            Serial.printf("[SimpleHTTP] ✓ Animation saved: %s (%zu bytes)\n",
                         filepath.c_str(), _uploadBuffer.size());
        } else {
            Serial.printf("[SimpleHTTP] ✗ Failed to save animation: %s\n", filepath.c_str());
        }

        // Clear upload buffer
        _uploadBuffer.clear();
    }
}

} // namespace Doki
