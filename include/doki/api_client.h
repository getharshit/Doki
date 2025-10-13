/**
 * @file api_client.h
 * @brief Generic HTTP API Client for Doki OS
 * 
 * Provides simple HTTP GET/POST methods for external API calls.
 * Handles retries, timeouts, and JSON parsing.
 * 
 * Example:
 *   String response;
 *   if (ApiClient::get("https://api.example.com/data", response)) {
 *       // Process response
 *   }
 */

#ifndef DOKI_API_CLIENT_H
#define DOKI_API_CLIENT_H

#include <Arduino.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>

namespace Doki {

/**
 * @brief API response structure
 */
struct ApiResponse {
    bool success;           // Whether request succeeded
    int statusCode;         // HTTP status code (200, 404, etc.)
    String body;            // Response body
    String error;           // Error message (if failed)
    uint32_t requestTime;   // Time taken for request (ms)
    
    ApiResponse() 
        : success(false), statusCode(0), requestTime(0) {}
};

/**
 * @brief API Client - Generic HTTP client for external APIs
 */
class ApiClient {
public:
    /**
     * @brief Perform HTTP GET request
     * 
     * @param url Full URL to request
     * @param response Output: Response structure
     * @param timeout Request timeout in ms (default: 5000)
     * @param retries Number of retry attempts (default: 3)
     * @return true if request succeeded
     * 
     * Example:
     *   ApiResponse resp;
     *   if (ApiClient::get("https://api.example.com/data", resp)) {
     *       Serial.println(resp.body);
     *   }
     */
    static bool get(const String& url, 
                   ApiResponse& response,
                   uint32_t timeout = 5000,
                   int retries = 3);
    
    /**
     * @brief Perform HTTP POST request
     * 
     * @param url Full URL to request
     * @param payload POST data (JSON string)
     * @param response Output: Response structure
     * @param timeout Request timeout in ms (default: 5000)
     * @param retries Number of retry attempts (default: 3)
     * @return true if request succeeded
     */
    static bool post(const String& url,
                    const String& payload,
                    ApiResponse& response,
                    uint32_t timeout = 5000,
                    int retries = 3);
    
    /**
     * @brief Parse JSON response
     * 
     * @param response API response
     * @param doc Output: JSON document
     * @return true if parsing succeeded
     * 
     * Example:
     *   JsonDocument doc;
     *   if (ApiClient::parseJson(resp, doc)) {
     *       String value = doc["key"].as<String>();
     *   }
     */
    static bool parseJson(const ApiResponse& response, JsonDocument& doc);

private:
    /**
     * @brief Perform HTTP request with retries
     * 
     * @param method HTTP method (GET or POST)
     * @param url URL to request
     * @param payload POST payload (empty for GET)
     * @param response Output: Response structure
     * @param timeout Request timeout
     * @param retries Number of retries
     * @return true if succeeded
     */
    static bool _performRequest(const String& method,
                               const String& url,
                               const String& payload,
                               ApiResponse& response,
                               uint32_t timeout,
                               int retries);
};

} // namespace Doki

#endif // DOKI_API_CLIENT_H