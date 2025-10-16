/**
 * @file setup_portal.h
 * @brief Setup Portal for Doki OS - WiFi configuration web interface
 *
 * Provides a captive portal for easy WiFi configuration on first boot.
 * Users connect to the AP, get redirected to the setup page, enter
 * their WiFi credentials, and the device restarts with the new config.
 *
 * Features:
 * - Captive portal (auto-redirect)
 * - WiFi network scanning
 * - Responsive web interface
 * - Save credentials to StorageManager
 * - Auto-restart after configuration
 *
 * Example:
 *   SetupPortal::begin();  // Start setup portal
 *
 *   // User connects, configures WiFi, device restarts automatically
 */

#ifndef DOKI_SETUP_PORTAL_H
#define DOKI_SETUP_PORTAL_H

#include <Arduino.h>
#include <ESPAsyncWebServer.h>
#include <DNSServer.h>
#include <ArduinoJson.h>

namespace Doki {

/**
 * @brief Setup Portal - WiFi configuration interface
 *
 * Singleton class that provides web-based WiFi setup
 */
class SetupPortal {
public:
    /**
     * @brief Start setup portal
     *
     * @param port HTTP server port (default: 80)
     * @return true if portal started successfully
     *
     * Starts a captive portal for WiFi configuration.
     * Users connecting to the AP will be redirected to the setup page.
     *
     * Example:
     *   SetupPortal::begin();
     *   Serial.println("Connect to 'DokiOS-Setup' to configure");
     */
    static bool begin(uint16_t port = 80);

    /**
     * @brief Stop setup portal
     *
     * Stops the captive portal and web server.
     */
    static void stop();

    /**
     * @brief Update DNS server
     *
     * Call this in loop() to handle DNS requests for captive portal.
     *
     * Example:
     *   void loop() {
     *       SetupPortal::update();
     *   }
     */
    static void update();

    /**
     * @brief Check if portal is running
     *
     * @return true if portal is active
     */
    static bool isRunning();

    /**
     * @brief Get setup URL
     *
     * @return Setup page URL (e.g., "http://192.168.4.1/setup")
     */
    static String getSetupURL();

private:
    static AsyncWebServer* _server;
    static DNSServer* _dnsServer;
    static bool _running;

    // DNS configuration
    static const byte DNS_PORT = 53;

    // HTTP handlers
    static void handleRoot(AsyncWebServerRequest* request);
    static void handleSetup(AsyncWebServerRequest* request);
    static void handleScanNetworks(AsyncWebServerRequest* request);
    static void handleSaveWiFi(AsyncWebServerRequest* request);
    static void handleStatus(AsyncWebServerRequest* request);
    static void handleNotFound(AsyncWebServerRequest* request);

    // HTML page generators
    static String generateSetupPage();
    static String generateSuccessPage();

    // Helper: Send JSON response
    static void sendJsonResponse(AsyncWebServerRequest* request,
                                 int statusCode,
                                 const JsonDocument& doc);
};

} // namespace Doki

#endif // DOKI_SETUP_PORTAL_H
