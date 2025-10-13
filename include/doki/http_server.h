/**
 * @file http_server.h
 * @brief HTTP Server for Doki OS - REST API and Web Dashboard
 * 
 * Provides HTTP endpoints to control Doki OS remotely:
 * - Load/unload apps
 * - Get system status
 * - View app registry
 * - Control via web dashboard
 * 
 * Example:
 *   HttpServer::begin(80);  // Start server on port 80
 *   
 *   // Access from browser:
 *   http://192.168.1.xxx/api/app/load?id=clock
 *   http://192.168.1.xxx/dashboard
 */

#ifndef DOKI_HTTP_SERVER_H
#define DOKI_HTTP_SERVER_H

#include <Arduino.h>
#include <ESPAsyncWebServer.h>
#include <ArduinoJson.h>

namespace Doki {

/**
 * @brief HTTP Server - REST API for remote control
 * 
 * Singleton class that manages the AsyncWebServer
 */
class HttpServer {
public:
    /**
     * @brief Start the HTTP server
     * 
     * @param port Port to listen on (default: 80)
     * @return true if server started successfully
     * 
     * Must be called after WiFi is connected.
     * 
     * Example:
     *   WiFi.begin(ssid, password);
     *   while (WiFi.status() != WL_CONNECTED) delay(500);
     *   HttpServer::begin(80);
     */
    static bool begin(uint16_t port = 80);
    
    /**
     * @brief Stop the HTTP server
     */
    static void stop();
    
    /**
     * @brief Check if server is running
     * 
     * @return true if server is running
     */
    static bool isRunning();
    
    /**
     * @brief Get server URL
     * 
     * @return Server URL (e.g., "http://192.168.1.100")
     */
    static String getServerUrl();
    
private:
    static AsyncWebServer* _server;
    static bool _running;
    
    // API endpoint handlers
    static void handleLoadApp(AsyncWebServerRequest* request);
    static void handleUnloadApp(AsyncWebServerRequest* request);
    static void handleGetCurrentApp(AsyncWebServerRequest* request);
    static void handleListApps(AsyncWebServerRequest* request);
    static void handleSystemStatus(AsyncWebServerRequest* request);
    static void handleDashboard(AsyncWebServerRequest* request);
    static void handleNotFound(AsyncWebServerRequest* request);
    
    // Helper: Create JSON response
    static void sendJsonResponse(AsyncWebServerRequest* request, 
                                 int statusCode, 
                                 const JsonDocument& doc);
    
    // Helper: Create error response
    static void sendErrorResponse(AsyncWebServerRequest* request, 
                                  int statusCode, 
                                  const char* message);
};

} // namespace Doki

#endif // DOKI_HTTP_SERVER_H