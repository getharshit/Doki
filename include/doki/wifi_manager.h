/**
 * @file wifi_manager.h
 * @brief WiFi Manager for Doki OS - Intelligent WiFi connection with fallback
 *
 * Manages WiFi connections with automatic fallback to Access Point mode
 * if station connection fails. Supports both AP and STA modes simultaneously
 * (hybrid mode) for maximum flexibility.
 *
 * Features:
 * - Load saved credentials from StorageManager
 * - Auto-connect to saved WiFi
 * - Fallback to AP mode if connection fails
 * - Hybrid AP+STA mode support
 * - Reconnection handling
 * - Connection callbacks
 *
 * Example:
 *   WiFiManager::init();
 *   if (WiFiManager::autoConnect()) {
 *       Serial.println("Connected to WiFi!");
 *   } else {
 *       Serial.println("Running in AP mode - setup required");
 *   }
 */

#ifndef DOKI_WIFI_MANAGER_H
#define DOKI_WIFI_MANAGER_H

#include <Arduino.h>
#include <WiFi.h>

namespace Doki {

/**
 * @brief WiFi connection status
 */
enum class WiFiStatus {
    DISCONNECTED,    // Not connected
    CONNECTING,      // Attempting to connect
    CONNECTED,       // Connected to station WiFi
    AP_MODE,         // Running as Access Point only
    HYBRID_MODE      // Both AP and Station active
};

/**
 * @brief WiFi Manager - Intelligent WiFi connection management
 *
 * Singleton class that handles all WiFi operations
 */
class WiFiManager {
public:
    /**
     * @brief Initialize WiFi manager
     *
     * @return true if initialization succeeded
     *
     * Prepares WiFi hardware but doesn't connect yet.
     *
     * Example:
     *   void setup() {
     *       WiFiManager::init();
     *   }
     */
    static bool init();

    /**
     * @brief Auto-connect to WiFi
     *
     * @param timeout Connection timeout in milliseconds (default: 10000)
     * @return true if connected to station WiFi
     *
     * Attempts to load saved credentials and connect. If no credentials
     * are saved or connection fails, starts in AP mode.
     *
     * Example:
     *   if (WiFiManager::autoConnect(10000)) {
     *       // Connected to WiFi, can access internet
     *   } else {
     *       // Running in AP mode, show setup screen
     *   }
     */
    static bool autoConnect(uint32_t timeout = 10000);

    /**
     * @brief Connect to specific WiFi network
     *
     * @param ssid Network name
     * @param password Network password
     * @param timeout Connection timeout in milliseconds
     * @return true if connection succeeded
     *
     * Example:
     *   if (WiFiManager::connectToWiFi("MyHome", "password123", 10000)) {
     *       StorageManager::saveWiFiCredentials("MyHome", "password123");
     *   }
     */
    static bool connectToWiFi(const String& ssid,
                              const String& password,
                              uint32_t timeout = 10000);

    /**
     * @brief Start Access Point mode
     *
     * @param ssid AP network name (default: "DokiOS-Setup")
     * @param password AP password (default: "doki1234")
     * @return true if AP started successfully
     *
     * Creates a WiFi access point that users can connect to
     * for initial setup.
     *
     * Example:
     *   WiFiManager::startAccessPoint("MyDevice-Setup", "setup123");
     */
    static bool startAccessPoint(const String& ssid = "DokiOS-Setup",
                                  const String& password = "doki1234");

    /**
     * @brief Enable hybrid mode (AP + Station)
     *
     * @param apSSID AP network name
     * @param apPassword AP password
     * @param staSSID Station network name
     * @param staPassword Station password
     * @param timeout Connection timeout
     * @return true if at least AP mode is active
     *
     * Starts both AP and Station modes simultaneously. AP always starts,
     * but station connection may fail.
     *
     * Example:
     *   WiFiManager::enableHybridMode(
     *       "DokiOS-Control", "doki1234",  // AP for dashboard
     *       "HomeWiFi", "password"          // Station for internet
     *   );
     */
    static bool enableHybridMode(const String& apSSID,
                                 const String& apPassword,
                                 const String& staSSID,
                                 const String& staPassword,
                                 uint32_t timeout = 10000);

    /**
     * @brief Disconnect from WiFi
     *
     * Disconnects from station WiFi but keeps AP mode if active.
     */
    static void disconnect();

    /**
     * @brief Stop Access Point
     *
     * Stops AP mode but keeps station connection if active.
     */
    static void stopAccessPoint();

    // ========================================
    // Status and Information
    // ========================================

    /**
     * @brief Get current WiFi status
     *
     * @return Current connection status
     */
    static WiFiStatus getStatus();

    /**
     * @brief Check if connected to station WiFi
     *
     * @return true if connected
     */
    static bool isConnected();

    /**
     * @brief Check if AP mode is active
     *
     * @return true if AP is running
     */
    static bool isAPMode();

    /**
     * @brief Get station IP address
     *
     * @return IP address (empty if not connected)
     */
    static String getIPAddress();

    /**
     * @brief Get AP IP address
     *
     * @return AP IP address (typically 192.168.4.1)
     */
    static String getAPIPAddress();

    /**
     * @brief Get connected SSID
     *
     * @return SSID of connected network (empty if not connected)
     */
    static String getSSID();

    /**
     * @brief Get AP SSID
     *
     * @return AP network name
     */
    static String getAPSSID();

    /**
     * @brief Get WiFi signal strength
     *
     * @return RSSI in dBm (0 if not connected)
     */
    static int getRSSI();

    /**
     * @brief Get number of connected clients (AP mode)
     *
     * @return Number of devices connected to AP
     */
    static int getAPClientCount();

    /**
     * @brief Print WiFi status
     *
     * Outputs detailed WiFi information to Serial
     */
    static void printStatus();

    // ========================================
    // Reconnection Handling
    // ========================================

    /**
     * @brief Handle WiFi reconnection
     *
     * Call periodically in loop() to handle automatic reconnection
     * if WiFi connection is lost.
     *
     * Example:
     *   void loop() {
     *       WiFiManager::handleReconnection();
     *   }
     */
    static void handleReconnection();

    /**
     * @brief Set reconnection enabled
     *
     * @param enabled true to enable auto-reconnection
     *
     * When enabled, WiFi will automatically reconnect if connection
     * is lost.
     */
    static void setAutoReconnect(bool enabled);

private:
    static bool _initialized;
    static WiFiStatus _status;
    static String _apSSID;
    static String _apPassword;
    static bool _autoReconnect;
    static uint32_t _lastReconnectAttempt;
    static const uint32_t RECONNECT_INTERVAL = 30000;  // 30 seconds

    // Helper: Wait for WiFi connection
    static bool _waitForConnection(uint32_t timeout);

    // Helper: Check if credentials are valid
    static bool _isValidSSID(const String& ssid);
};

} // namespace Doki

#endif // DOKI_WIFI_MANAGER_H
