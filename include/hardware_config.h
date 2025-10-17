/**
 * Hardware Configuration for Doki OS
 *
 * Centralizes hardware-specific constants for easy board adaptation.
 * Change these values to match your specific hardware setup.
 */

#ifndef HARDWARE_CONFIG_H
#define HARDWARE_CONFIG_H

// ==========================================
// Display Hardware
// ==========================================

// Display Dimensions
#define DISPLAY_WIDTH                   240
#define DISPLAY_HEIGHT                  320

// SPI Pins (shared by all displays)
#define SPI_MOSI_PIN                    37
#define SPI_SCLK_PIN                    36

// Display 0 Pins
#define DISPLAY_0_CS_PIN                33
#define DISPLAY_0_DC_PIN                15
#define DISPLAY_0_RST_PIN               16

// Display 1 Pins
#define DISPLAY_1_CS_PIN                34
#define DISPLAY_1_DC_PIN                17
#define DISPLAY_1_RST_PIN               18

// Display 2 Pins (for future expansion)
#define DISPLAY_2_CS_PIN                35      // Update when adding 3rd display
#define DISPLAY_2_DC_PIN                21      // Update when adding 3rd display
#define DISPLAY_2_RST_PIN               22      // Update when adding 3rd display

// ==========================================
// Display Configuration
// ==========================================

// Number of active displays (1-3)
#define DISPLAY_COUNT                   2

// LVGL Buffer Configuration
#define LVGL_BUFFER_LINES               40      // Lines per buffer (increase for smoother rendering)
#define LVGL_USE_DOUBLE_BUFFER          true    // Double buffering enabled

// Display Orientation (0=Portrait, 1=Landscape, 2=Portrait180, 3=Landscape180)
#define DISPLAY_ROTATION                0

// ==========================================
// Memory Configuration
// ==========================================

// PSRAM Settings
#define USE_PSRAM_FOR_BUFFERS           true    // Allocate LVGL buffers in PSRAM
#define USE_PSRAM_FOR_JS_HEAP           false   // Allocate JavaScript heap in PSRAM (Phase 3 optimization)

// JavaScript Engine
#define JS_HEAP_SIZE_KB                 128     // Duktape heap size per app (kilobytes)
#define JS_CODE_MAX_SIZE_BYTES          16384   // Max JavaScript source code size (16 KB)

// Animation System
#define ANIMATION_POOL_SIZE_KB          1024    // Total PSRAM for animations (1MB)
#define ANIMATION_FRAME_BUFFER_SIZE_KB  512     // Frame buffer size per animation
#define MAX_CONCURRENT_ANIMATIONS       2       // Maximum animations playing simultaneously
#define ANIMATION_CACHE_SIZE_KB         200     // Metadata and state cache

// ==========================================
// Network Configuration
// ==========================================

// WebSocket
#define WS_RECONNECT_INTERVAL_MS        500     // Fast reconnection

// HTTP Server
#define HTTP_SERVER_PORT                80      // Web dashboard port

// MQTT
#define MQTT_DEFAULT_PORT               1883    // Standard MQTT port
#define MQTT_MAX_PACKET_SIZE            256     // MQTT packet buffer size

// ==========================================
// Performance Tuning
// ==========================================

// Task Stack Sizes (bytes)
#define TASK_STACK_NETWORK              4096    // Network operations task
#define TASK_STACK_DISPLAY              8192    // Display rendering task
#define TASK_STACK_NTP_SYNC             4096    // NTP background sync
#define TASK_STACK_WEBSOCKET            4096    // WebSocket handling

// Task Priorities (0-25, higher = more priority)
#define TASK_PRIORITY_DISPLAY           2       // Display rendering priority
#define TASK_PRIORITY_NETWORK           1       // Network operations priority
#define TASK_PRIORITY_NTP               1       // NTP sync priority (low, background)

// Task Core Assignment (0 or 1)
#define TASK_CORE_NETWORK               0       // Core 0 for network operations
#define TASK_CORE_DISPLAY               1       // Core 1 for display rendering

// FreeRTOS
#define FREERTOS_TICK_RATE_HZ           1000    // OS tick rate (default is usually fine)

// ==========================================
// Serial Debug
// ==========================================

#define SERIAL_BAUD_RATE                115200
#define ENABLE_SERIAL_DEBUG             true

// ==========================================
// System Limits
// ==========================================

// File System
#define MAX_FILENAME_LENGTH             64
#define MAX_PATH_LENGTH                 256

// Apps
#define MAX_CONCURRENT_APPS             3       // Maximum apps that can be loaded (one per display)
#define MAX_APP_NAME_LENGTH             32

#endif // HARDWARE_CONFIG_H
