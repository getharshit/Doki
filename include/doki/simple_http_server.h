/**
 * @file simple_http_server.h
 * @brief Simple HTTP Server for Doki OS Display Management
 */

#ifndef DOKI_SIMPLE_HTTP_SERVER_H
#define DOKI_SIMPLE_HTTP_SERVER_H

#include <Arduino.h>
#include <ESPAsyncWebServer.h>
#include <ArduinoJson.h>

namespace Doki {

/**
 * @brief Simple HTTP Server for app switching
 *
 * Provides a web dashboard to control which apps are displayed
 * on each display.
 */
class SimpleHttpServer {
public:
    /**
     * @brief Start the HTTP server
     * @param port Port to listen on (default: 80)
     * @return true if started successfully
     */
    static bool begin(uint16_t port = 80);

    /**
     * @brief Stop the HTTP server
     */
    static void stop();

    /**
     * @brief Check if server is running
     * @return true if running
     */
    static bool isRunning();

    /**
     * @brief Set callback for loading apps
     * @param callback Function that loads an app: bool loadApp(uint8_t displayId, String appId)
     */
    static void setLoadAppCallback(bool (*callback)(uint8_t, const String&));

    /**
     * @brief Set callback for getting display status
     * @param callback Function that returns app name and uptime: void getStatus(uint8_t displayId, String& appId, uint32_t& uptime)
     */
    static void setStatusCallback(void (*callback)(uint8_t, String&, uint32_t&));

private:
    static AsyncWebServer* _server;
    static bool _running;
    static bool (*_loadAppCallback)(uint8_t, const String&);
    static void (*_statusCallback)(uint8_t, String&, uint32_t&);

    // HTTP request handlers
    static void handleGetApps(AsyncWebServerRequest* request);
    static void handleLoadApp(AsyncWebServerRequest* request);
    static void handleGetStatus(AsyncWebServerRequest* request);
    static void handleDashboard(AsyncWebServerRequest* request);
    static void handleMediaInfo(AsyncWebServerRequest* request);
    static void handleMediaDelete(AsyncWebServerRequest* request);
    static void handleUploadJS(AsyncWebServerRequest* request);
    static void handleMediaUpload(AsyncWebServerRequest* request,
                                  const String& filename,
                                  size_t index,
                                  uint8_t* data,
                                  size_t len,
                                  bool final);

    // HTML generation
    static String generateDashboardHTML();

    // Upload state management
    static uint8_t _uploadDisplayId;
    static String _uploadMediaType;
    static std::vector<uint8_t> _uploadBuffer;
};

} // namespace Doki

#endif // DOKI_SIMPLE_HTTP_SERVER_H
