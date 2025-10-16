# Doki OS - Dual Display System Documentation

## Overview

**Doki OS** is a dual-display embedded system built on the ESP32-S3 platform with dual ST7789 240x320 TFT displays. It features a modular application framework with web-based control, independent app management per display, and real-time data services (weather, time, system monitoring).

### Key Features

- **Dual Independent Displays**: Two 240x320 ST7789 displays controlled independently
- **Web Dashboard**: HTTP-based control interface with real-time status updates
- **Modular App Framework**: Plugin-based app architecture with lifecycle management
- **Hybrid WiFi Mode**: Simultaneous Access Point (for dashboard) and Station mode (for internet)
- **Real-time Services**: Weather API integration, NTP time sync, system monitoring
- **LVGL Graphics**: Hardware-accelerated UI with PSRAM optimization
- **Non-blocking Architecture**: FreeRTOS task-based design for smooth operation

### System Architecture

```
┌─────────────────────────────────────────────────────────────┐
│                        Doki OS System                        │
├─────────────────────────────────────────────────────────────┤
│  Web Dashboard (HTTP Server)                                 │
│  ├─ Control Panel                                            │
│  ├─ App Selection per Display                                │
│  └─ System Status Monitor                                    │
├─────────────────────────────────────────────────────────────┤
│  Display Manager (Dual ST7789)                               │
│  ├─ Display 0: Independent LVGL Context                      │
│  └─ Display 1: Independent LVGL Context                      │
├─────────────────────────────────────────────────────────────┤
│  Application Framework                                       │
│  ├─ App Base (Lifecycle Management)                          │
│  ├─ App Manager (Registry & Loading)                         │
│  └─ Apps: Clock, Weather, SysInfo, Hello, Goodbye, Blank     │
├─────────────────────────────────────────────────────────────┤
│  Services Layer                                              │
│  ├─ Weather Service (WeatherAPI.com)                         │
│  ├─ Time Manager (NTP Client)                                │
│  └─ HTTP Server (AsyncWebServer)                             │
├─────────────────────────────────────────────────────────────┤
│  Hardware Abstraction                                        │
│  ├─ ST7789 Driver (SPI Communication)                        │
│  ├─ LVGL (Graphics Library)                                  │
│  └─ WiFi (Hybrid AP+STA Mode)                                │
└─────────────────────────────────────────────────────────────┘
```

## Documentation Structure

1. **[Hardware Setup](./01-hardware-setup.md)** - Hardware configuration, pinout, and wiring
2. **[Platform Configuration](./02-platform-configuration.md)** - PlatformIO settings and build configuration
3. **[System Architecture](./03-system-architecture.md)** - Core system design and components
4. **[Application Framework](./04-application-framework.md)** - App lifecycle and development guide
5. **[Display Management](./05-display-management.md)** - Dual display control and LVGL integration
6. **[Services](./06-services.md)** - Weather, time, and HTTP services
7. **[Web Dashboard](./07-web-dashboard.md)** - HTTP API and dashboard interface
8. **[App Development Guide](./08-app-development.md)** - Creating custom apps
9. **[API Reference](./09-api-reference.md)** - Complete API documentation

## Quick Start

### Prerequisites

- PlatformIO IDE (VS Code extension recommended)
- ESP32-S3 development board (Waveshare ESP32-S3R2 or compatible)
- Two ST7789 240x320 TFT displays
- WiFi network for internet access (optional, for weather data)

### Building and Flashing

```bash
# Clone the repository
cd /path/to/Doki

# Build the project
pio run

# Upload to ESP32-S3
pio run --target upload

# Monitor serial output
pio device monitor -b 115200
```

### First Boot

1. On first boot, the system creates a WiFi access point: **DokiOS-Control**
2. Connect to it using password: **doki1234**
3. Open browser and navigate to: **http://192.168.4.1/dashboard**
4. You'll see the web dashboard with both displays and available apps

### Connecting to Internet (for Weather)

Edit the WiFi credentials in [src/main.cpp](../src/main.cpp):

```cpp
const char* STATION_SSID = "YourWiFiName";
const char* STATION_PASSWORD = "YourWiFiPassword";
```

The system runs in hybrid mode: it creates an AP for the dashboard while connecting to your WiFi for internet access.

## Project Structure

```
Doki/
├── platformio.ini              # Build configuration
├── include/
│   ├── config.h               # System configuration
│   ├── lv_conf.h              # LVGL configuration
│   └── doki/                  # Doki framework headers
│       ├── app_base.h         # Base app class
│       ├── app_manager.h      # App registry and lifecycle
│       ├── display_manager.h  # Multi-display management
│       ├── weather_service.h  # Weather API integration
│       └── http_server.h      # HTTP server interface
├── src/
│   ├── main.cpp               # Main program entry
│   ├── st7789_driver.cpp      # Display driver
│   ├── time_manager.cpp       # NTP time management
│   ├── doki/                  # Doki framework implementation
│   │   ├── app_base.cpp
│   │   ├── app_manager.cpp
│   │   ├── display_manager.cpp
│   │   ├── weather_service.cpp
│   │   └── http_server.cpp
│   ├── apps/                  # Application modules
│   │   ├── clock_app/         # Clock with NTP sync
│   │   ├── weather_app/       # Weather display
│   │   ├── sysinfo_app/       # System information
│   │   ├── hello_app/         # Demo app
│   │   ├── goodbye_app/       # Demo app
│   │   └── blank_app/         # Image screensaver
│   └── assets/                # Binary assets (images, fonts)
│       ├── dance.c            # Embedded GIF/image data
│       └── dance.h
├── data/                      # SPIFFS filesystem (if needed)
└── docs/                      # Documentation (this folder)
```

## Hardware Specifications

### ESP32-S3 Board
- **MCU**: ESP32-S3 (Dual-core Xtensa LX7, 240MHz)
- **RAM**: 512KB SRAM + 2MB PSRAM (QSPI)
- **Flash**: 16MB QSPI Flash
- **WiFi**: 2.4GHz 802.11 b/g/n
- **Bluetooth**: BLE 5.0

### Display Specifications
- **Controller**: ST7789V
- **Resolution**: 240x320 pixels
- **Interface**: SPI (shared MOSI/SCLK, separate CS/DC/RST)
- **Color Depth**: RGB565 (16-bit)

### Pin Configuration

| Function | Display 0 | Display 1 | Shared |
|----------|-----------|-----------|--------|
| CS       | GPIO 33   | GPIO 34   | -      |
| DC       | GPIO 15   | GPIO 17   | -      |
| RST      | GPIO 16   | GPIO 18   | -      |
| MOSI     | -         | -         | GPIO 37|
| SCLK     | -         | -         | GPIO 36|

## Available Applications

### Clock App
- Real-time clock with NTP synchronization
- 12-hour format with AM/PM indicator
- Day progress bar and visual indicators
- Non-blocking background time updates
- Uptime display

### Weather App
- Real-time weather data from WeatherAPI.com
- Temperature, feels-like, humidity, wind speed
- Temperature-based color coding
- 10-minute caching to reduce API calls
- Animated weather icon

### System Info App
- Heap and PSRAM memory usage
- WiFi signal strength and IP address
- System uptime
- Real-time metrics

### Screensaver (Blank App)
- Displays embedded image/GIF
- Low CPU usage
- Optional zoom animation

### Demo Apps
- Hello App: Simple greeting display
- Goodbye App: Farewell message

## Web Dashboard Features

- **Real-time Display Status**: Shows current app on each display
- **App Switching**: One-click app loading per display
- **System Metrics**: Memory, WiFi, uptime
- **Responsive Design**: Works on desktop and mobile
- **Auto-refresh**: Updates every 2 seconds

## Development

### Adding a New App

1. Create app header in `src/apps/myapp/myapp.h`
2. Inherit from `Doki::DokiApp`
3. Implement lifecycle methods: `onCreate()`, `onStart()`, `onUpdate()`, `onDestroy()`
4. Register in `main.cpp` `createApp()` function
5. Add to dashboard app list

See [App Development Guide](./08-app-development.md) for detailed instructions.

## Performance

### Memory Usage
- **Heap**: ~150KB used (of 512KB)
- **PSRAM**: ~400KB used (of 2MB) - primarily for LVGL buffers
- **Flash**: ~1.2MB used (of 16MB)

### Display Performance
- **Refresh Rate**: ~30 FPS per display
- **App Switch Time**: 100-300ms
- **LVGL Rendering**: Hardware-optimized with double-buffering

### Network Performance
- **HTTP Response Time**: <50ms for API calls
- **Weather Fetch**: ~500ms (cached for 10 minutes)
- **NTP Sync**: Background task, non-blocking

## Troubleshooting

### Displays Not Working
- Check SPI connections and pin configuration
- Verify 3.3V power supply
- Check `platformio.ini` for correct board settings

### WiFi Connection Fails
- Verify SSID and password in `main.cpp`
- Check WiFi signal strength
- System will work in AP-only mode without internet

### Weather Not Updating
- Ensure WiFi station mode is connected
- Check WeatherAPI.com API key validity
- Verify internet connectivity

### Slow Performance
- Check PSRAM is enabled (`BOARD_HAS_PSRAM`)
- Reduce LVGL animation complexity
- Monitor memory usage in System Info app

## License

This project is open-source. See LICENSE file for details.

## Credits

- **LVGL**: Graphics library (https://lvgl.io)
- **WeatherAPI.com**: Weather data provider
- **PlatformIO**: Development platform
- **ESP32 Arduino**: Framework

## Support

For issues, questions, or contributions, please refer to the project repository.

---

**Version**: 0.1.0 - Hybrid Mode
**Last Updated**: 2025-01-16
