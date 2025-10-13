/**
 * @file api_client.cpp
 * @brief Implementation of API Client for Doki OS
 */

#include "doki/api_client.h"
#include <WiFi.h>

namespace Doki {

// ========================================
// Public Methods
// ========================================

bool ApiClient::get(const String& url, 
                   ApiResponse& response,
                   uint32_t timeout,
                   int retries) {
    return _performRequest("GET", url, "", response, timeout, retries);
}

bool ApiClient::post(const String& url,
                    const String& payload,
                    ApiResponse& response,
                    uint32_t timeout,
                    int retries) {
    return _performRequest("POST", url, payload, response, timeout, retries);
}

bool ApiClient::parseJson(const ApiResponse& response, JsonDocument& doc) {
    if (!response.success) {
        Serial.println("[ApiClient] Cannot parse JSON from failed response");
        return false;
    }
    
    DeserializationError error = deserializeJson(doc, response.body);
    
    if (error) {
        Serial.printf("[ApiClient] JSON parse error: %s\n", error.c_str());
        return false;
    }
    
    return true;
}

// ========================================
// Private Methods
// ========================================

bool ApiClient::_performRequest(const String& method,
                               const String& url,
                               const String& payload,
                               ApiResponse& response,
                               uint32_t timeout,
                               int retries) {
    // Check WiFi connection
    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("[ApiClient] Error: WiFi not connected");
        response.success = false;
        response.error = "WiFi not connected";
        return false;
    }
    
    Serial.printf("[ApiClient] %s %s\n", method.c_str(), url.c_str());
    
    HTTPClient http;
    
    // Try request with retries
    for (int attempt = 1; attempt <= retries; attempt++) {
        if (attempt > 1) {
            Serial.printf("[ApiClient] Retry attempt %d/%d\n", attempt, retries);
            delay(1000);  // Wait 1 second before retry
        }
        
        uint32_t startTime = millis();
        
        // Begin HTTP connection
        http.begin(url);
        http.setTimeout(timeout);
        
        // Set headers
        if (method == "POST") {
            http.addHeader("Content-Type", "application/json");
        }
        
        // Perform request
        int httpCode;
        if (method == "GET") {
            httpCode = http.GET();
        } else if (method == "POST") {
            httpCode = http.POST(payload);
        } else {
            Serial.printf("[ApiClient] Error: Unsupported method '%s'\n", method.c_str());
            http.end();
            response.success = false;
            response.error = "Unsupported HTTP method";
            return false;
        }
        
        response.requestTime = millis() - startTime;
        response.statusCode = httpCode;
        
        // Check response
        if (httpCode > 0) {
            // HTTP response received
            response.body = http.getString();
            
            if (httpCode >= 200 && httpCode < 300) {
                // Success
                Serial.printf("[ApiClient] ✓ Success (HTTP %d, %d ms)\n", 
                             httpCode, response.requestTime);
                response.success = true;
                http.end();
                return true;
            } else {
                // HTTP error
                Serial.printf("[ApiClient] HTTP error %d: %s\n", 
                             httpCode, response.body.c_str());
                response.error = "HTTP " + String(httpCode);
                
                // Don't retry on client errors (4xx)
                if (httpCode >= 400 && httpCode < 500) {
                    Serial.println("[ApiClient] Client error, not retrying");
                    break;
                }
            }
        } else {
            // Connection error
            Serial.printf("[ApiClient] Connection error: %s\n", 
                         http.errorToString(httpCode).c_str());
            response.error = http.errorToString(httpCode);
        }
        
        http.end();
    }
    
    // All retries failed
    Serial.printf("[ApiClient] ✗ Request failed after %d attempts\n", retries);
    response.success = false;
    return false;
}

} // namespace Doki