/**
 * @file weather_service.cpp
 * @brief Implementation of Weather Service for Doki OS
 */

#include "doki/weather_service.h"
#include "doki/api_client.h"
#include <ArduinoJson.h>

namespace Doki {

// ========================================
// Static Member Initialization
// ========================================

String WeatherService::_apiKey = "";
WeatherData WeatherService::_cachedData;
String WeatherService::_cachedLocation = "";

// ========================================
// Public Methods
// ========================================

void WeatherService::init(const char* apiKey) {
    _apiKey = String(apiKey);
    Serial.printf("[WeatherService] Initialized with API key: %s...\n", 
                  _apiKey.substring(0, 8).c_str());
}

bool WeatherService::getCurrentWeather(const String& location, WeatherData& data) {
    // Check if we have valid cached data for this location
    if (_cachedLocation == location && isCacheValid()) {
        Serial.printf("[WeatherService] Using cached data for '%s' (age: %lu ms)\n",
                      location.c_str(), millis() - _cachedData.lastUpdated);
        data = _cachedData;
        return true;
    }
    
    // Fetch fresh data
    return refreshWeather(location, data);
}

bool WeatherService::isCacheValid() {
    if (!_cachedData.valid) {
        return false;
    }
    
    uint32_t age = millis() - _cachedData.lastUpdated;
    return age < CACHE_DURATION_MS;
}

bool WeatherService::refreshWeather(const String& location, WeatherData& data) {
    Serial.printf("[WeatherService] Fetching fresh weather for '%s'\n", location.c_str());
    
    bool success = _fetchWeather(location, data);
    
    if (success) {
        // Update cache
        _cachedData = data;
        _cachedLocation = location;
        
        Serial.printf("[WeatherService] ✓ Weather updated: %.1f°C, %s\n",
                      data.tempC, data.condition.c_str());
    } else {
        Serial.println("[WeatherService] ✗ Failed to fetch weather");
    }
    
    return success;
}

WeatherData WeatherService::getCachedData() {
    return _cachedData;
}

void WeatherService::clearCache() {
    Serial.println("[WeatherService] Cache cleared");
    _cachedData = WeatherData();
    _cachedLocation = "";
}

// ========================================
// Private Methods
// ========================================

bool WeatherService::_fetchWeather(const String& location, WeatherData& data) {
    if (_apiKey.isEmpty()) {
        Serial.println("[WeatherService] Error: API key not set");
        return false;
    }
    
    // Build API URL
    String url = "http://api.weatherapi.com/v1/current.json";
    url += "?key=" + _apiKey;
    url += "&q=" + location;
    url += "&aqi=no";
    
    // Make API request
    ApiResponse response;
    if (!ApiClient::get(url, response, 10000, 3)) {
        Serial.println("[WeatherService] API request failed");
        return false;
    }
    
    // Parse JSON response
    return _parseWeatherJson(response.body, data);
}

bool WeatherService::_parseWeatherJson(const String& json, WeatherData& data) {
    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, json);
    
    if (error) {
        Serial.printf("[WeatherService] JSON parse error: %s\n", error.c_str());
        return false;
    }
    
    // Check for API error
    if (doc["error"].is<JsonObject>()) {
        String errorMsg = doc["error"]["message"].as<String>();
        Serial.printf("[WeatherService] API error: %s\n", errorMsg.c_str());
        return false;
    }
    
    // Parse location
    JsonObject location = doc["location"];
    if (location) {
        data.location = location["name"].as<String>();
        data.region = location["region"].as<String>();
        data.country = location["country"].as<String>();
    }
    
    // Parse current weather
    JsonObject current = doc["current"];
    if (!current) {
        Serial.println("[WeatherService] No 'current' data in response");
        return false;
    }
    
    // Temperature
    data.tempC = current["temp_c"].as<float>();
    data.tempF = current["temp_f"].as<float>();
    data.feelsLikeC = current["feelslike_c"].as<float>();
    data.feelsLikeF = current["feelslike_f"].as<float>();
    
    // Condition
    JsonObject condition = current["condition"];
    if (condition) {
        data.condition = condition["text"].as<String>();
        data.conditionCode = condition["code"].as<int>();
    }
    
    // Wind
    data.windKph = current["wind_kph"].as<float>();
    data.windMph = current["wind_mph"].as<float>();
    data.windDir = current["wind_dir"].as<String>();
    
    // Humidity
    data.humidity = current["humidity"].as<int>();
    
    // Metadata
    data.lastUpdated = millis();
    data.valid = true;
    
    Serial.println("[WeatherService] Weather data parsed successfully:");
    Serial.printf("  Location: %s, %s, %s\n", 
                  data.location.c_str(), data.region.c_str(), data.country.c_str());
    Serial.printf("  Temperature: %.1f°C (feels like %.1f°C)\n", 
                  data.tempC, data.feelsLikeC);
    Serial.printf("  Condition: %s (code: %d)\n", 
                  data.condition.c_str(), data.conditionCode);
    Serial.printf("  Wind: %.1f km/h %s\n", data.windKph, data.windDir.c_str());
    Serial.printf("  Humidity: %d%%\n", data.humidity);
    
    return true;
}

} // namespace Doki