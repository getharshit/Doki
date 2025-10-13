/**
 * System Configuration
 * Place in: include/config.h
 */

#ifndef CONFIG_H
#define CONFIG_H

// ==========================================
// WiFi Configuration
// ==========================================
#define WIFI_SSID "Abhi"        // Replace with your WiFi name
#define WIFI_PASSWORD "" // Replace with your WiFi password
#define WIFI_RETRY_ATTEMPTS 10             // Number of connection attempts
#define WIFI_RETRY_DELAY_MS 1000          // Delay between attempts (ms)

// ==========================================
// Time Configuration
// ==========================================
#define NTP_SERVER "pool.ntp.org"         // NTP server
#define NTP_SERVER_BACKUP "time.google.com" // Backup NTP server
#define GMT_OFFSET_SEC 19800              // UTC+5:30 for India (5.5 * 3600)
#define DAYLIGHT_OFFSET_SEC 0             // No daylight saving in India
#define NTP_UPDATE_INTERVAL_MS 3600000    // Update every hour (3600000 ms)

// ==========================================
// Display Configuration
// ==========================================
#define DISPLAY_COUNT 1                   // Phase 2: Single display, will scale to 3
#define DISPLAY_REFRESH_MS 1000           // Update display every 1 second

// Time Display Format
#define TIME_FORMAT_12H true              // 12-hour format
#define SHOW_SECONDS true                 // Show seconds in time
#define DATE_FORMAT_DMY true              // DD/MM/YYYY format

// ==========================================
// System Configuration
// ==========================================
#define SERIAL_BAUD_RATE 115200
#define SYSTEM_STATUS_UPDATE_MS 30000     // Print status every 30 seconds

#endif // CONFIG_H