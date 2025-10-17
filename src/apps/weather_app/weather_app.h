/**
 * @file weather_app.h
 * @brief OPTIMIZED Weather App with reduced animations
 * 
 * Changes from original:
 * - Removed continuous background gradient animations
 * - Simplified weather icon animation (less CPU intensive)
 * - Update at 10 FPS instead of constantly
 * - Static bars instead of animated bars
 */

#ifndef WEATHER_APP_H
#define WEATHER_APP_H

#include "doki/app_base.h"
#include "doki/lvgl_helpers.h"
#include "timing_constants.h"

using namespace Doki;
#include "doki/weather_service.h"
#include <math.h>

class WeatherApp : public Doki::DokiApp {
public:
    WeatherApp() : DokiApp("weather", "Weather") {}
    
    void onCreate() override {
        log("Creating OPTIMIZED Weather App...");
        
        // Static background (no animations)
        lv_obj_set_style_bg_color(getScreen(), lv_color_hex(0x000000), 0);
        
        // Title with icon
        _titleLabel = lv_label_create(getScreen());
        lv_label_set_text(_titleLabel, "Weather");
        lv_obj_align(_titleLabel, LV_ALIGN_TOP_MID, 0, 10);
        lv_obj_set_style_text_font(_titleLabel, &lv_font_montserrat_18, 0);
        lv_obj_set_style_text_color(_titleLabel, lv_color_hex(0x667eea), 0);
        
        // Location
        _locationLabel = lv_label_create(getScreen());
        lv_label_set_text(_locationLabel, "Loading...");
        lv_obj_align(_locationLabel, LV_ALIGN_TOP_MID, 0, 40);
        lv_obj_set_style_text_font(_locationLabel, &lv_font_montserrat_12, 0);
        lv_obj_set_style_text_color(_locationLabel, lv_color_hex(0x666666), 0);
        
        // Temperature
        _tempLabel = lv_label_create(getScreen());
        lv_label_set_text(_tempLabel, "--°C");
        lv_obj_align(_tempLabel, LV_ALIGN_CENTER, 0, -40);
        lv_obj_set_style_text_font(_tempLabel, &lv_font_montserrat_48, 0);
        lv_obj_set_style_text_color(_tempLabel, lv_color_hex(0xFF6B35), 0);
        
        // Simple weather icon circle (minimal animation)
        _weatherIcon = lv_obj_create(getScreen());
        lv_obj_set_size(_weatherIcon, 60, 60);
        lv_obj_align(_weatherIcon, LV_ALIGN_CENTER, -70, -40);
        lv_obj_set_style_radius(_weatherIcon, LV_RADIUS_CIRCLE, 0);
        lv_obj_set_style_bg_color(_weatherIcon, lv_color_hex(0xFFD700), 0);
        lv_obj_set_style_bg_opa(_weatherIcon, LV_OPA_30, 0);
        lv_obj_set_style_border_width(_weatherIcon, 2, 0);
        lv_obj_set_style_border_color(_weatherIcon, lv_color_hex(0xFFD700), 0);
        lv_obj_set_style_border_opa(_weatherIcon, LV_OPA_50, 0);
        
        // Feels like
        _feelsLikeLabel = lv_label_create(getScreen());
        lv_label_set_text(_feelsLikeLabel, "Feels like --°C");
        lv_obj_align(_feelsLikeLabel, LV_ALIGN_CENTER, 0, 15);
        lv_obj_set_style_text_font(_feelsLikeLabel, &lv_font_montserrat_12, 0);
        lv_obj_set_style_text_color(_feelsLikeLabel, lv_color_hex(0x888888), 0);
        
        // Condition
        _conditionLabel = lv_label_create(getScreen());
        lv_label_set_text(_conditionLabel, "---");
        lv_obj_align(_conditionLabel, LV_ALIGN_CENTER, 0, 35);
        lv_obj_set_style_text_font(_conditionLabel, &lv_font_montserrat_16, 0);
        lv_obj_set_style_text_color(_conditionLabel, lv_color_hex(0x333333), 0);
        
        // Static humidity bar (no animation)
        _humidityBar = lv_bar_create(getScreen());
        lv_obj_set_size(_humidityBar, 100, 8);
        lv_obj_align(_humidityBar, LV_ALIGN_BOTTOM_LEFT, 20, -50);
        lv_bar_set_range(_humidityBar, 0, 100);
        lv_obj_set_style_bg_color(_humidityBar, lv_color_hex(0xe5e7eb), 0);
        lv_obj_set_style_bg_color(_humidityBar, lv_color_hex(0x3B82F6), LV_PART_INDICATOR);
        lv_obj_set_style_radius(_humidityBar, 4, 0);
        
        _humidityLabel = lv_label_create(getScreen());
        lv_label_set_text(_humidityLabel, "H: --%");
        lv_obj_align(_humidityLabel, LV_ALIGN_BOTTOM_LEFT, 20, -30);
        lv_obj_set_style_text_font(_humidityLabel, &lv_font_montserrat_10, 0);
        
        // Static wind speed bar (no animation)
        _windBar = lv_bar_create(getScreen());
        lv_obj_set_size(_windBar, 100, 8);
        lv_obj_align(_windBar, LV_ALIGN_BOTTOM_RIGHT, -20, -50);
        lv_bar_set_range(_windBar, 0, 50);
        lv_obj_set_style_bg_color(_windBar, lv_color_hex(0xe5e7eb), 0);
        lv_obj_set_style_bg_color(_windBar, lv_color_hex(0x10b981), LV_PART_INDICATOR);
        lv_obj_set_style_radius(_windBar, 4, 0);
        
        _windLabel = lv_label_create(getScreen());
        lv_label_set_text(_windLabel, "W: -- km/h");
        lv_obj_align(_windLabel, LV_ALIGN_BOTTOM_RIGHT, -20, -30);
        lv_obj_set_style_text_font(_windLabel, &lv_font_montserrat_10, 0);
        
        // Status
        _statusLabel = lv_label_create(getScreen());
        lv_label_set_text(_statusLabel, "Fetching...");
        lv_obj_align(_statusLabel, LV_ALIGN_BOTTOM_MID, 0, -5);
        lv_obj_set_style_text_font(_statusLabel, &lv_font_montserrat_10, 0);
        lv_obj_set_style_text_color(_statusLabel, lv_color_hex(0x888888), 0);
        
        _lastUpdate = 0;
        _lastWeatherFetch = 0;
        _lastAnimation = 0;
        _location = "Mumbai";
        _animPhase = 0;
        
        // Simple fade in (no animation system)
        lv_obj_set_style_opa(getScreen(), LV_OPA_COVER, 0);
    }
    
    void onStart() override {
        log("Weather App started!");
        fetchWeather();
    }
    
    void onUpdate() override {
        uint32_t now = millis();
        
        // Fetch weather every 10 minutes
        if (now - _lastWeatherFetch >= 600000 || _lastWeatherFetch == 0) {
            fetchWeather();
            _lastWeatherFetch = now;
        }
        
        // Update status every 1 second
        if (now - _lastUpdate >= 1000) {
            updateStatus();
            _lastUpdate = now;
        }
        
        // Simple icon pulse at 10 FPS (100ms interval)
        if (now - _lastAnimation >= 100) {
            animateWeatherIcon();
            _lastAnimation = now;
        }
    }
    
    void onPause() override {
        log("Weather App paused");
    }
    
    void onDestroy() override {
        log("Weather App destroyed");
    }

private:
    lv_obj_t* _titleLabel;
    lv_obj_t* _locationLabel;
    lv_obj_t* _tempLabel;
    lv_obj_t* _weatherIcon;
    lv_obj_t* _feelsLikeLabel;
    lv_obj_t* _conditionLabel;
    lv_obj_t* _humidityBar;
    lv_obj_t* _humidityLabel;
    lv_obj_t* _windBar;
    lv_obj_t* _windLabel;
    lv_obj_t* _statusLabel;
    
    String _location;
    Doki::WeatherData _currentWeather;
    uint32_t _lastUpdate;
    uint32_t _lastWeatherFetch;
    uint32_t _lastAnimation;
    float _animPhase;
    
    void fetchWeather() {
        log(("Fetching weather for " + _location).c_str());
        
        if (Doki::WeatherService::getCurrentWeather(_location, _currentWeather)) {
            updateWeatherDisplay();
            log("Weather updated successfully");
        } else {
            log("Failed to fetch weather");
            lv_label_set_text(_statusLabel, "Failed to fetch");
        }
    }
    
    void updateWeatherDisplay() {
        if (!_currentWeather.valid) return;
        
        // Location
        String loc = _currentWeather.location;
        if (loc.length() > 20) loc = loc.substring(0, 20);
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
        
        // Temperature-based coloring
        lv_color_t tempColor;
        lv_color_t iconColor;
        if (_currentWeather.tempC >= 35) {
            tempColor = lv_color_hex(0xFF4444);
            iconColor = lv_color_hex(0xFF4444);
        } else if (_currentWeather.tempC >= 25) {
            tempColor = lv_color_hex(0xFF6B35);
            iconColor = lv_color_hex(0xFFD700);
        } else if (_currentWeather.tempC >= 15) {
            tempColor = lv_color_hex(0x4CAF50);
            iconColor = lv_color_hex(0x4CAF50);
        } else {
            tempColor = lv_color_hex(0x2196F3);
            iconColor = lv_color_hex(0x2196F3);
        }
        
        lv_obj_set_style_text_color(_tempLabel, tempColor, 0);
        lv_obj_set_style_bg_color(_weatherIcon, iconColor, 0);
        lv_obj_set_style_border_color(_weatherIcon, iconColor, 0);
        
        // Humidity (NO animation, just set value)
        lv_bar_set_value(_humidityBar, _currentWeather.humidity, LV_ANIM_OFF);
        char humidityBuf[16];
        snprintf(humidityBuf, sizeof(humidityBuf), "H: %d%%", _currentWeather.humidity);
        lv_label_set_text(_humidityLabel, humidityBuf);
        
        // Wind (NO animation, just set value)
        int windValue = (int)_currentWeather.windKph;
        if (windValue > 50) windValue = 50;
        lv_bar_set_value(_windBar, windValue, LV_ANIM_OFF);
        char windBuf[32];
        snprintf(windBuf, sizeof(windBuf), "W: %.0f km/h", _currentWeather.windKph);
        lv_label_set_text(_windLabel, windBuf);
    }
    
    void updateStatus() {
        if (!_currentWeather.valid) return;
        
        uint32_t age = (millis() - _currentWeather.lastUpdated) / 1000;
        char statusBuf[32];
        
        if (age < 60) {
            snprintf(statusBuf, sizeof(statusBuf), "Updated %lus ago", age);
        } else if (age < 3600) {
            snprintf(statusBuf, sizeof(statusBuf), "Updated %lum ago", age / 60);
        } else {
            snprintf(statusBuf, sizeof(statusBuf), "Updated %luh ago", age / 3600);
        }
        
        lv_label_set_text(_statusLabel, statusBuf);
    }
    
    void animateWeatherIcon() {
        // Simple pulsing icon - manual update
        _animPhase += 0.15f;
        if (_animPhase > 6.28f) _animPhase = 0;
        
        // Subtle pulse (only opacity changes)
        uint8_t iconOpa = 25 + (uint8_t)(10 * sin(_animPhase));
        lv_obj_set_style_bg_opa(_weatherIcon, iconOpa, 0);
    }
};

#endif // WEATHER_APP_H