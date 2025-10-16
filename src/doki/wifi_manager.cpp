/**
 * @file wifi_manager.cpp
 * @brief Implementation of WiFiManager
 */

#include "doki/wifi_manager.h"
#include "doki/storage_manager.h"

namespace Doki {

// ========================================
// Static Member Initialization
// ========================================

bool WiFiManager::_initialized = false;
WiFiStatus WiFiManager::_status = WiFiStatus::DISCONNECTED;
String WiFiManager::_apSSID = "";
String WiFiManager::_apPassword = "";
bool WiFiManager::_autoReconnect = true;
uint32_t WiFiManager::_lastReconnectAttempt = 0;

// ========================================
// Initialization
// ========================================

bool WiFiManager::init() {
    if (_initialized) {
        Serial.println("[WiFiManager] Already initialized");
        return true;
    }

    Serial.println("╔═══════════════════════════════════════════════════╗");
    Serial.println("║          WiFi Manager Initialization              ║");
    Serial.println("╚═══════════════════════════════════════════════════╝");

    // Set WiFi mode to off initially
    WiFi.mode(WIFI_OFF);
    delay(100);

    _initialized = true;
    _status = WiFiStatus::DISCONNECTED;

    Serial.println("[WiFiManager] ✓ WiFi Manager initialized");
    return true;
}

// ========================================
// Connection Management
// ========================================

bool WiFiManager::autoConnect(uint32_t timeout) {
    if (!_initialized) {
        Serial.println("[WiFiManager] Error: Not initialized!");
        return false;
    }

    Serial.println("\n[WiFiManager] Auto-connect starting...");

    // Try to load saved credentials
    String ssid, password;
    if (!StorageManager::loadWiFiCredentials(ssid, password)) {
        Serial.println("[WiFiManager] No saved WiFi credentials found");
        Serial.println("[WiFiManager] Starting in AP mode for setup");

        startAccessPoint();
        return false;
    }

    // Try to connect with saved credentials
    Serial.printf("[WiFiManager] Attempting to connect to '%s'...\n", ssid.c_str());

    if (connectToWiFi(ssid, password, timeout)) {
        Serial.println("[WiFiManager] ✓ Connected successfully!");
        return true;
    }

    // Connection failed, start AP mode
    Serial.println("[WiFiManager] Connection failed");
    Serial.println("[WiFiManager] Starting in AP mode for setup");

    startAccessPoint();
    return false;
}

bool WiFiManager::connectToWiFi(const String& ssid,
                                 const String& password,
                                 uint32_t timeout) {
    if (!_initialized) return false;

    if (!_isValidSSID(ssid)) {
        Serial.println("[WiFiManager] Error: Invalid SSID");
        return false;
    }

    Serial.printf("[WiFiManager] Connecting to '%s'...\n", ssid.c_str());

    // Set station mode
    WiFi.mode(WIFI_STA);
    delay(100);

    // Start connection
    _status = WiFiStatus::CONNECTING;
    WiFi.begin(ssid.c_str(), password.c_str());

    // Wait for connection
    if (_waitForConnection(timeout)) {
        _status = WiFiStatus::CONNECTED;
        Serial.println("[WiFiManager] ✓ Connected to WiFi!");
        Serial.printf("[WiFiManager] IP Address: %s\n", WiFi.localIP().toString().c_str());
        Serial.printf("[WiFiManager] Signal: %d dBm\n", WiFi.RSSI());
        return true;
    }

    // Connection failed
    _status = WiFiStatus::DISCONNECTED;
    WiFi.disconnect();
    Serial.println("[WiFiManager] ✗ Connection failed");
    return false;
}

bool WiFiManager::startAccessPoint(const String& ssid, const String& password) {
    if (!_initialized) return false;

    Serial.printf("[WiFiManager] Starting Access Point: '%s'...\n", ssid.c_str());

    _apSSID = ssid;
    _apPassword = password;

    // Set AP mode
    WiFi.mode(WIFI_AP);
    delay(100);

    // Start AP
    bool success = WiFi.softAP(ssid.c_str(), password.c_str());

    if (success) {
        _status = WiFiStatus::AP_MODE;
        Serial.println("[WiFiManager] ✓ Access Point started!");
        Serial.printf("[WiFiManager] SSID: %s\n", ssid.c_str());
        Serial.printf("[WiFiManager] Password: %s\n", password.c_str());
        Serial.printf("[WiFiManager] IP Address: %s\n", WiFi.softAPIP().toString().c_str());
        Serial.println("[WiFiManager] Connect to this network to configure WiFi");
        return true;
    }

    Serial.println("[WiFiManager] ✗ Failed to start Access Point");
    return false;
}

bool WiFiManager::enableHybridMode(const String& apSSID,
                                    const String& apPassword,
                                    const String& staSSID,
                                    const String& staPassword,
                                    uint32_t timeout) {
    if (!_initialized) return false;

    Serial.println("[WiFiManager] Enabling Hybrid Mode (AP + Station)...");

    _apSSID = apSSID;
    _apPassword = apPassword;

    // Set hybrid mode
    WiFi.mode(WIFI_AP_STA);
    delay(100);

    // Start AP
    bool apStarted = WiFi.softAP(apSSID.c_str(), apPassword.c_str());
    if (!apStarted) {
        Serial.println("[WiFiManager] ✗ Failed to start AP");
        return false;
    }

    Serial.println("[WiFiManager] ✓ Access Point started");
    Serial.printf("[WiFiManager]   SSID: %s\n", apSSID.c_str());
    Serial.printf("[WiFiManager]   AP IP: %s\n", WiFi.softAPIP().toString().c_str());

    // Try to connect to station WiFi
    Serial.printf("[WiFiManager] Connecting to station: '%s'...\n", staSSID.c_str());
    WiFi.begin(staSSID.c_str(), staPassword.c_str());

    if (_waitForConnection(timeout)) {
        _status = WiFiStatus::HYBRID_MODE;
        Serial.println("[WiFiManager] ✓ Station connected!");
        Serial.printf("[WiFiManager]   Station IP: %s\n", WiFi.localIP().toString().c_str());
        Serial.printf("[WiFiManager]   Signal: %d dBm\n", WiFi.RSSI());
        Serial.println("[WiFiManager] ✓ Hybrid mode active!");
        return true;
    }

    // Station connection failed, but AP is still running
    _status = WiFiStatus::AP_MODE;
    Serial.println("[WiFiManager] ⚠ Station connection failed");
    Serial.println("[WiFiManager] Running in AP-only mode");
    return true;  // Still successful (AP is running)
}

void WiFiManager::disconnect() {
    if (!_initialized) return;

    Serial.println("[WiFiManager] Disconnecting from WiFi...");
    WiFi.disconnect();

    if (_status == WiFiStatus::HYBRID_MODE) {
        _status = WiFiStatus::AP_MODE;
    } else {
        _status = WiFiStatus::DISCONNECTED;
    }
}

void WiFiManager::stopAccessPoint() {
    if (!_initialized) return;

    Serial.println("[WiFiManager] Stopping Access Point...");
    WiFi.softAPdisconnect(true);

    if (_status == WiFiStatus::HYBRID_MODE) {
        _status = WiFiStatus::CONNECTED;
    } else {
        _status = WiFiStatus::DISCONNECTED;
    }

    _apSSID = "";
    _apPassword = "";
}

// ========================================
// Status and Information
// ========================================

WiFiStatus WiFiManager::getStatus() {
    return _status;
}

bool WiFiManager::isConnected() {
    return (_status == WiFiStatus::CONNECTED || _status == WiFiStatus::HYBRID_MODE)
           && (WiFi.status() == WL_CONNECTED);
}

bool WiFiManager::isAPMode() {
    return _status == WiFiStatus::AP_MODE || _status == WiFiStatus::HYBRID_MODE;
}

String WiFiManager::getIPAddress() {
    if (!isConnected()) return "";
    return WiFi.localIP().toString();
}

String WiFiManager::getAPIPAddress() {
    if (!isAPMode()) return "";
    return WiFi.softAPIP().toString();
}

String WiFiManager::getSSID() {
    if (!isConnected()) return "";
    return WiFi.SSID();
}

String WiFiManager::getAPSSID() {
    return _apSSID;
}

int WiFiManager::getRSSI() {
    if (!isConnected()) return 0;
    return WiFi.RSSI();
}

int WiFiManager::getAPClientCount() {
    if (!isAPMode()) return 0;
    return WiFi.softAPgetStationNum();
}

void WiFiManager::printStatus() {
    if (!_initialized) {
        Serial.println("[WiFiManager] Not initialized");
        return;
    }

    Serial.println("\n┌─────────────────────────────────────────────────┐");
    Serial.println("│              WiFi Status                        │");
    Serial.println("├─────────────────────────────────────────────────┤");

    // Status
    const char* statusStr[] = {"Disconnected", "Connecting", "Connected", "AP Mode", "Hybrid Mode"};
    Serial.printf("│ Status:           %-26s │\n", statusStr[(int)_status]);

    // Station info
    if (isConnected()) {
        Serial.printf("│ Station SSID:     %-26s │\n", getSSID().c_str());
        Serial.printf("│ Station IP:       %-26s │\n", getIPAddress().c_str());
        Serial.printf("│ Signal Strength:  %-21d dBm │\n", getRSSI());
    }

    // AP info
    if (isAPMode()) {
        Serial.printf("│ AP SSID:          %-26s │\n", getAPSSID().c_str());
        Serial.printf("│ AP IP:            %-26s │\n", getAPIPAddress().c_str());
        Serial.printf("│ Connected Clients: %-25d │\n", getAPClientCount());
    }

    Serial.println("└─────────────────────────────────────────────────┘\n");
}

// ========================================
// Reconnection Handling
// ========================================

void WiFiManager::handleReconnection() {
    if (!_initialized || !_autoReconnect) return;

    // Only try to reconnect if we were connected before
    if (_status != WiFiStatus::CONNECTED && _status != WiFiStatus::HYBRID_MODE) {
        return;
    }

    // Check if still connected
    if (WiFi.status() == WL_CONNECTED) {
        return;  // Still connected, all good
    }

    // Connection lost
    uint32_t now = millis();
    if (now - _lastReconnectAttempt < RECONNECT_INTERVAL) {
        return;  // Too soon to retry
    }

    _lastReconnectAttempt = now;

    Serial.println("[WiFiManager] Connection lost, attempting to reconnect...");

    // Try to load saved credentials and reconnect
    String ssid, password;
    if (StorageManager::loadWiFiCredentials(ssid, password)) {
        connectToWiFi(ssid, password, 5000);
    }
}

void WiFiManager::setAutoReconnect(bool enabled) {
    _autoReconnect = enabled;
    Serial.printf("[WiFiManager] Auto-reconnect: %s\n", enabled ? "Enabled" : "Disabled");
}

// ========================================
// Private Helper Functions
// ========================================

bool WiFiManager::_waitForConnection(uint32_t timeout) {
    uint32_t startTime = millis();

    Serial.print("[WiFiManager] Connecting");

    while (WiFi.status() != WL_CONNECTED) {
        if (millis() - startTime >= timeout) {
            Serial.println(" ✗ Timeout");
            return false;
        }

        delay(500);
        Serial.print(".");
    }

    Serial.println(" ✓");
    return true;
}

bool WiFiManager::_isValidSSID(const String& ssid) {
    if (ssid.isEmpty()) return false;
    if (ssid.length() > 32) return false;  // Max SSID length
    return true;
}

} // namespace Doki
