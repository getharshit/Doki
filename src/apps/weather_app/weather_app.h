/**
 * @file weather_app.h
 * @brief Weather App with real-time weather data for Doki OS
 */

#ifndef WEATHER_APP_H
#define WEATHER_APP_H

#include "doki/app_base.h"
#include "doki/weather_service.h"

class WeatherApp : public Doki::DokiApp {
public:
    WeatherApp() : DokiApp("weather", "Weather") {}
    
    void onCreate() override {
        log("Creating Weather App UI...");
        
        // Title
        _titleLabel = lv_label_create(getScreen());
        lv_label_set_text(_titleLabel, "Weather");
        lv_obj_align(_titleLabel, LV_ALIGN_TOP_MID, 0, 10);
        lv_obj_set_style_text_font(_titleLabel, &lv_font_montserrat_18, 0);
        lv_obj_set_style_text_color(_titleLabel, lv_color_hex(0x667eea), 0);
        
        // Location
        _locationLabel = lv_label_create(getScreen());
        lv_label_set_text(_locationLabel, "Loading...");
        lv_obj_align(_locationLabel, LV_ALIGN_TOP_MID, 0, 40);
        lv_obj_set_style_text_font(_locationLabel, &lv_font_montserrat_14, 0);
        
        // Temperature (big and bold)
        _tempLabel = lv_label_create(getScreen());
        lv_label_set_text(_tempLabel, "--°C");
        lv_obj_align(_tempLabel, LV_ALIGN_CENTER, 0, -30);
        lv_obj_set_style_text_font(_tempLabel, &lv_font_montserrat_48, 0);
        lv_obj_set_style_text_color(_tempLabel, lv_color_hex(0xFF6B35), 0);
        
        // Feels like
        _feelsLikeLabel = lv_label_create(getScreen());
        lv_label_set_text(_feelsLikeLabel, "Feels like --°C");
        lv_obj_align(_feelsLikeLabel, LV_ALIGN_CENTER, 0, 25);
        lv_obj_set_style_text_font(_feelsLikeLabel, &lv_font_montserrat_12, 0);
        lv_obj_set_style_text_color(_feelsLikeLabel, lv_color_hex(0x888888), 0);
        
        // Condition
        _conditionLabel = lv_label_create(getScreen());
        lv_label_set_text(_conditionLabel, "---");
        lv_obj_align(_conditionLabel, LV_ALIGN_CENTER, 0, 45);
        lv_obj_set_style_text_font(_conditionLabel, &lv_font_montserrat_16, 0);
        
        // Humidity & Wind (side by side)
        _humidityLabel = lv_label_create(getScreen());
        lv_label_set_text(_humidityLabel, "💧 --%");
        lv_obj_align(_humidityLabel, LV_ALIGN_BOTTOM_LEFT, 20, -40);
        lv_obj_set_style_text_font(_humidityLabel, &lv_font_montserrat_12, 0);
        
        _windLabel = lv_label_create(getScreen());
        lv_label_set_text(_windLabel, "🌬 -- km/h");
        lv_obj_align(_windLabel, LV_ALIGN_BOTTOM_RIGHT, -20, -40);
        lv_obj_set_style_text_font(_windLabel, &lv_font_montserrat_12, 0);
        
        // Status/Last updated
        _statusLabel = lv_label_create(getScreen());
        lv_label_set_text(_statusLabel, "Fetching weather...");
        lv_obj_align(_statusLabel, LV_ALIGN_BOTTOM_MID, 0, -10);
        lv_obj_set_style_text_font(_statusLabel, &lv_font_montserrat_10, 0);
        lv_obj_set_style_text_color(_statusLabel, lv_color_hex(0x888888), 0);
        
        _lastUpdate = 0;
        _lastWeatherFetch = 0;
        _location = "Mumbai";  // Default location
    }
    
    void onStart() override {
        log("Weather App started!");
        // Fetch weather immediately
        fetchWeather();
    }
    
    void onUpdate() override {
        uint32_t now = millis();
        
        // Fetch weather every 10 minutes (respects cache)
        if (now - _lastWeatherFetch >= 600000 || _lastWeatherFetch == 0) {
            fetchWeather();
            _lastWeatherFetch = now;
        }
        
        // Update status display every second
        if (now - _lastUpdate >= 1000) {
            updateStatus();
            _lastUpdate = now;
        }
    }
    
    void onPause() override {
        log("Weather App paused");
    }
    
    void onDestroy() override {
        log("Weather App destroyed");
    }
    
    /**
     * @brief Set location for weather
     * @param location City name (e.g., "Mumbai", "London")
     */
    void setLocation(const String& location) {
        _location = location;
        log(("Location set to: " + location).c_str());
        fetchWeather();
    }

private:
    lv_obj_t* _titleLabel;
    lv_obj_t* _locationLabel;
    lv_obj_t* _tempLabel;
    lv_obj_t* _feelsLikeLabel;
    lv_obj_t* _conditionLabel;
    lv_obj_t* _humidityLabel;
    lv_obj_t* _windLabel;
    lv_obj_t* _statusLabel;
    
    String _location;
    Doki::WeatherData _currentWeather;
    uint32_t _lastUpdate;
    uint32_t _lastWeatherFetch;
    
    void fetchWeather() {
        log(("Fetching weather for " + _location).c_str());
        
        if (Doki::WeatherService::getCurrentWeather(_location, _currentWeather)) {
            updateWeatherDisplay();
            log("Weather updated successfully");
        } else {
            log("Failed to fetch weather");
            lv_label_set_text(_statusLabel, "Failed to fetch weather");
        }
    }
    
    void updateWeatherDisplay() {
        if (!_currentWeather.valid) {
            return;
        }
        
        // Location
        String loc = _currentWeather.location + ", " + _currentWeather.country;
        lv_label_set_text(_locationLabel, loc.c_str());
        
        // Temperature
        char tempBuf[16];
        snprintf(tempBuf, sizeof(tempBuf), "%.0f°C", _currentWeather.tempC);
        lv_label_set_text(_tempLabel, tempBuf);
        
        // Feels like
        char feelsLikeBuf[32];
        snprintf(feelsLikeBuf, sizeof(feelsLikeBuf), 
                 "Feels like %.0f°C", _currentWeather.feelsLikeC);
        lv_label_set_text(_feelsLikeLabel, feelsLikeBuf);
        
        // Condition
        lv_label_set_text(_conditionLabel, _currentWeather.condition.c_str());
        
        // Color temperature based on value
        if (_currentWeather.tempC >= 35) {
            lv_obj_set_style_text_color(_tempLabel, lv_color_hex(0xFF4444), 0);  // Hot - Red
        } else if (_currentWeather.tempC >= 25) {
            lv_obj_set_style_text_color(_tempLabel, lv_color_hex(0xFF6B35), 0);  // Warm - Orange
        } else if (_currentWeather.tempC >= 15) {
            lv_obj_set_style_text_color(_tempLabel, lv_color_hex(0x4CAF50), 0);  // Pleasant - Green
        } else {
            lv_obj_set_style_text_color(_tempLabel, lv_color_hex(0x2196F3), 0);  // Cold - Blue
        }
        
        // Humidity
        char humidityBuf[16];
        snprintf(humidityBuf, sizeof(humidityBuf), "💧 %d%%", _currentWeather.humidity);
        lv_label_set_text(_humidityLabel, humidityBuf);
        
        // Wind
        char windBuf[32];
        snprintf(windBuf, sizeof(windBuf), "🌬 %.0f km/h %s", 
                 _currentWeather.windKph, _currentWeather.windDir.c_str());
        lv_label_set_text(_windLabel, windBuf);
        
        updateStatus();
    }
    
    void updateStatus() {
        if (!_currentWeather.valid) {
            return;
        }
        
        uint32_t age = (millis() - _currentWeather.lastUpdated) / 1000;
        char statusBuf[64];
        
        if (age < 60) {
            snprintf(statusBuf, sizeof(statusBuf), "Updated %lu sec ago", age);
        } else if (age < 3600) {
            snprintf(statusBuf, sizeof(statusBuf), "Updated %lu min ago", age / 60);
        } else {
            snprintf(statusBuf, sizeof(statusBuf), "Updated %lu hr ago", age / 3600);
        }
        
        lv_label_set_text(_statusLabel, statusBuf);
    }
};

#endif // WEATHER_APP_H