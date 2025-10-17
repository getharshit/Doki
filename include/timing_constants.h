/**
 * Timing Constants for Doki OS
 *
 * Centralizes all timing-related constants for easy tuning and consistency.
 * All values in milliseconds unless otherwise noted.
 */

#ifndef TIMING_CONSTANTS_H
#define TIMING_CONSTANTS_H

// ==========================================
// Update Intervals
// ==========================================

// Display & UI Updates
#define UPDATE_INTERVAL_CLOCK_MS        1000    // Clock refresh (1 second)
#define UPDATE_INTERVAL_SYSINFO_MS      2000    // System info refresh (2 seconds)
#define UPDATE_INTERVAL_DISPLAY_MS      1000    // General display update (1 second)

// Network Services
#define UPDATE_INTERVAL_WEATHER_MS      600000  // Weather fetch (10 minutes)
#define UPDATE_INTERVAL_NTP_MS          3600000 // NTP time sync (1 hour)
#define UPDATE_INTERVAL_NTP_RETRY_MS    60000   // NTP retry if failed (1 minute)
#define UPDATE_INTERVAL_MQTT_CHECK_MS   100     // MQTT message check (100ms)
#define UPDATE_INTERVAL_WS_POLL_MS      20      // WebSocket poll (20ms)

// Multi-Display Coordination
#define UPDATE_INTERVAL_DISPLAY_SYNC_MS 5000    // Inter-display sync (5 seconds)

// Animation System
#define UPDATE_INTERVAL_ANIMATION_MS    33      // Animation frame update (~30 FPS)
#define UPDATE_INTERVAL_ANIMATION_60FPS_MS 16   // 60 FPS option
#define UPDATE_INTERVAL_ANIMATION_24FPS_MS 42   // 24 FPS option (cinematic)
#define UPDATE_INTERVAL_ANIMATION_15FPS_MS 67   // 15 FPS option (low power)

// ==========================================
// Timeouts
// ==========================================

// Network Timeouts
#define TIMEOUT_HTTP_REQUEST_MS         5000    // HTTP GET/POST timeout
#define TIMEOUT_MQTT_CONNECT_MS         10000   // MQTT connection timeout
#define TIMEOUT_MQTT_SOCKET_SEC         5       // MQTT socket timeout (seconds)
#define TIMEOUT_WIFI_CONNECT_MS         10000   // WiFi connection timeout
#define TIMEOUT_NTP_SYNC_MS             10000   // NTP sync timeout

// Application Timeouts
#define TIMEOUT_APP_LOAD_MS             5000    // Max time to load app
#define TIMEOUT_JS_EXECUTION_MS         1000    // JavaScript execution limit
#define TIMEOUT_ANIMATION_LOAD_MS       5000    // Max time to load animation
#define TIMEOUT_ANIMATION_DOWNLOAD_MS   30000   // Max time to download sprite (30s)

// ==========================================
// Delays
// ==========================================

// Initialization Delays (prevents watchdog timeout)
#define DELAY_MQTT_INIT_MS              3000    // Wait before MQTT connect
#define DELAY_WEATHER_INIT_MS           5000    // Wait before first weather fetch
#define DELAY_WS_INIT_MS                8000    // Wait before WebSocket connect
#define DELAY_NTP_INIT_MS               2000    // Wait for WiFi stabilization

// Retry Delays
#define DELAY_WIFI_RETRY_MS             1000    // Between WiFi reconnection attempts
#define DELAY_HTTP_RETRY_MS             5000    // Between HTTP retries
#define DELAY_WS_RECONNECT_MS           500     // WebSocket reconnect interval
#define DELAY_WS_CLEANUP_MS             100     // WebSocket disconnect cleanup delay

// UI Delays
#define DELAY_ANIMATION_MS              500     // Standard animation duration
#define DELAY_FADE_MS                   2000    // Title fade duration
#define DELAY_SPLASH_MS                 3000    // Boot splash display time

// ==========================================
// Conversion Constants
// ==========================================

#define SECONDS_TO_MS(s)                ((s) * 1000)
#define MINUTES_TO_MS(m)                ((m) * 60 * 1000)
#define HOURS_TO_MS(h)                  ((h) * 60 * 60 * 1000)

// Common time values
#define ONE_SECOND_MS                   1000
#define ONE_MINUTE_MS                   60000
#define ONE_HOUR_MS                     3600000
#define ONE_DAY_MS                      86400000

#endif // TIMING_CONSTANTS_H
