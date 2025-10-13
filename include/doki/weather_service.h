/**
 * @file weather_service.h
 * @brief Weather Service for Doki OS - WeatherAPI.com integration
 * 
 * Fetches weather data from WeatherAPI.com with caching.
 * Your API Key: 3183db8ec2fe4abfa2c133226251310
 * 
 * Example:
 *   WeatherData data;
 *   if (WeatherService::getCurrentWeather("Mumbai", data)) {
 *       Serial.printf("Temperature: %.1f°C\n", data.tempC);
 *       Serial.printf("Condition: %s\n", data.condition.c_str());
 *   }
 */

#ifndef DOKI_WEATHER_SERVICE_H
#define DOKI_WEATHER_SERVICE_H

#include <Arduino.h>

namespace Doki {

/**
 * @brief Weather data structure
 */
struct WeatherData {
    // Location
    String location;        // City name (e.g., "Mumbai")
    String region;          // Region/state
    String country;         // Country
    
    // Current conditions
    float tempC;            // Temperature in Celsius
    float tempF;            // Temperature in Fahrenheit
    String condition;       // Weather condition (e.g., "Sunny", "Cloudy")
    int conditionCode;      // Condition code (for icons)
    
    // Additional data
    int humidity;           // Humidity percentage
    float windKph;          // Wind speed in km/h
    float windMph;          // Wind speed in mph
    String windDir;         // Wind direction (e.g., "NE")
    float feelsLikeC;       // Feels like temperature in C
    float feelsLikeF;       // Feels like temperature in F
    
    // Metadata
    uint32_t lastUpdated;   // When data was fetched (millis())
    bool valid;             // Whether data is valid
    
    WeatherData() 
        : tempC(0), tempF(0), conditionCode(0)
        , humidity(0), windKph(0), windMph(0)
        , feelsLikeC(0), feelsLikeF(0)
        , lastUpdated(0), valid(false) {}
};

/**
 * @brief Weather Service - WeatherAPI.com integration
 * 
 * Singleton class for fetching weather data
 */
class WeatherService {
public:
    /**
     * @brief Initialize weather service
     * 
     * @param apiKey Your WeatherAPI.com API key
     * 
     * Must be called before using the service.
     * 
     * Example:
     *   WeatherService::init("3183db8ec2fe4abfa2c133226251310");
     */
    static void init(const char* apiKey);
    
    /**
     * @brief Get current weather for a location
     * 
     * @param location City name (e.g., "Mumbai", "London", "New York")
     * @param data Output: Weather data structure
     * @return true if data fetched successfully
     * 
     * Data is cached for 10 minutes to reduce API calls.
     * 
     * Example:
     *   WeatherData weather;
     *   if (WeatherService::getCurrentWeather("Mumbai", weather)) {
     *       Serial.printf("Temp: %.1f°C, %s\n", 
     *                     weather.tempC, weather.condition.c_str());
     *   }
     */
    static bool getCurrentWeather(const String& location, WeatherData& data);
    
    /**
     * @brief Check if cached data is still valid
     * 
     * @return true if cache is valid (less than 10 minutes old)
     */
    static bool isCacheValid();
    
    /**
     * @brief Force refresh weather data (ignore cache)
     * 
     * @param location City name
     * @param data Output: Weather data
     * @return true if data fetched successfully
     */
    static bool refreshWeather(const String& location, WeatherData& data);
    
    /**
     * @brief Get cached weather data
     * 
     * @return Cached weather data (may be invalid)
     */
    static WeatherData getCachedData();
    
    /**
     * @brief Clear cache
     */
    static void clearCache();

private:
    static String _apiKey;
    static WeatherData _cachedData;
    static String _cachedLocation;
    static const uint32_t CACHE_DURATION_MS = 600000;  // 10 minutes
    
    /**
     * @brief Fetch weather from API
     * 
     * @param location City name
     * @param data Output: Weather data
     * @return true if fetch succeeded
     */
    static bool _fetchWeather(const String& location, WeatherData& data);
    
    /**
     * @brief Parse WeatherAPI.com JSON response
     * 
     * @param json JSON response string
     * @param data Output: Weather data
     * @return true if parsing succeeded
     */
    static bool _parseWeatherJson(const String& json, WeatherData& data);
};

} // namespace Doki

#endif // DOKI_WEATHER_SERVICE_H