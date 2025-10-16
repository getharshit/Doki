# Platform Configuration

This document explains the PlatformIO configuration and build settings for the Doki OS project.

## platformio.ini Overview

The project is configured for the ESP32-S3 platform with specific optimizations for dual-display operation and PSRAM usage.

## Configuration File

Location: [platformio.ini](../platformio.ini)

```ini
[env:waveshare-esp32-s3]
platform = espressif32
board = esp32-s3-devkitc-1
framework = arduino
```

### Platform and Board

- **Platform**: `espressif32` - Espressif ESP32 platform with latest toolchain
- **Board**: `esp32-s3-devkitc-1` - Generic ESP32-S3 development board
- **Framework**: `arduino` - Arduino framework for ease of development

## Upload Configuration

```ini
upload_speed = 921600
monitor_speed = 115200
```

- **Upload Speed**: 921600 baud (fast firmware upload)
- **Monitor Speed**: 115200 baud (standard serial communication)

## Board Build Configuration

### MCU and CPU Settings

```ini
board_build.mcu = esp32s3
board_build.f_cpu = 240000000L
```

- **MCU**: ESP32-S3 (dual-core Xtensa LX7)
- **CPU Frequency**: 240MHz (maximum performance)

### Flash Configuration

```ini
board_build.flash_mode = qio
board_build.flash_size = 16MB
board_build.partitions = default_16MB.csv
board_build.arduino.memory_type = qio_qspi
```

- **Flash Mode**: QIO (Quad I/O) for faster read/write
- **Flash Size**: 16MB (large storage for assets and firmware)
- **Partitions**: Custom 16MB partition scheme
- **Memory Type**: QSPI mode for optimal performance

### PSRAM Configuration (Critical)

```ini
-DBOARD_HAS_PSRAM
-DCONFIG_SPIRAM_MODE_QUAD=1
-DCONFIG_SPIRAM_TYPE_ESPPSRAM16=1
```

This project **requires PSRAM** for LVGL display buffers. The ESP32-S3R2 has 2MB of QSPI PSRAM.

**Why PSRAM is Critical:**
- Each display requires two frame buffers (~38KB each)
- LVGL internal memory pool (~64KB)
- App resources and UI elements
- Total PSRAM usage: ~400KB

**Without PSRAM, the system will fail to initialize displays.**

## Build Flags

### USB Serial Configuration

```ini
-DARDUINO_USB_MODE=1
-DARDUINO_USB_CDC_ON_BOOT=1
```

- Enables USB CDC (Communication Device Class) for serial communication
- Serial output available immediately on boot via USB

### LVGL Configuration

```ini
-DLV_CONF_INCLUDE_SIMPLE
-DLV_CONF_PATH="${PROJECT_DIR}/include/lv_conf.h"
```

- Includes custom LVGL configuration from [include/lv_conf.h](../include/lv_conf.h)
- Optimized for ESP32-S3 with PSRAM

### Display Configuration

```ini
-DDISPLAY_WIDTH=240
-DDISPLAY_HEIGHT=320
-DDISPLAY_ROTATION=0
```

- **Width**: 240 pixels (portrait mode)
- **Height**: 320 pixels
- **Rotation**: 0° (no rotation)

### Asset Embedding

```ini
-DEMBEDDED_GIF=1
-I include
```

- Embeds binary assets (images, GIFs) into firmware
- Includes `include/` directory for headers

## Library Dependencies

```ini
lib_deps =
    lvgl/lvgl@^8.3.11
    arduino-libraries/NTPClient@^3.2.1
    bblanchon/ArduinoJson@^7.2.1
    esphome/ESPAsyncWebServer-esphome@^3.0.0
```

### LVGL (Graphics Library)
- **Version**: 8.3.11
- **Purpose**: UI rendering and graphics
- **Documentation**: https://docs.lvgl.io/8.3/

### NTPClient (Network Time Protocol)
- **Version**: 3.2.1
- **Purpose**: Time synchronization from internet
- **Repository**: https://github.com/arduino-libraries/NTPClient

### ArduinoJson (JSON Parser)
- **Version**: 7.2.1
- **Purpose**: Parsing weather API responses and HTTP requests
- **Documentation**: https://arduinojson.org/

### ESPAsyncWebServer (HTTP Server)
- **Version**: 3.0.0
- **Purpose**: Non-blocking HTTP server for web dashboard
- **Repository**: https://github.com/esphome/ESPAsyncWebServer

## Library Configuration

```ini
lib_ldf_mode = deep+
```

- **Library Dependency Finder Mode**: `deep+`
- Performs deep inspection of dependencies
- Ensures all library dependencies are resolved correctly

## Filesystem Configuration

```ini
board_build.filesystem = spiffs
board_upload.flash_size = 16MB
```

- **Filesystem**: SPIFFS (SPI Flash File System)
- **Purpose**: Store configuration files or web assets (currently unused)
- **Future Use**: Can store HTML/CSS for dashboard if needed

## Build Process

### Compiling the Project

```bash
# Build the project
pio run

# Clean build (from scratch)
pio run --target clean
pio run
```

### Uploading to ESP32-S3

```bash
# Upload firmware
pio run --target upload

# Upload and monitor
pio run --target upload && pio device monitor
```

### Serial Monitor

```bash
# Monitor serial output
pio device monitor -b 115200
```

### Expected Build Output

```
RAM:   [==        ]  18.5% (used 96404 bytes from 520192 bytes)
Flash: [====      ]  35.2% (used 1155421 bytes from 3276800 bytes)
```

- **RAM Usage**: ~100KB (leaving ~400KB free)
- **Flash Usage**: ~1.2MB (plenty of space for future expansion)

## Memory Optimization

### Compilation Flags

The following optimization flags are implicitly set by Arduino-ESP32:

- **Optimization Level**: `-Os` (optimize for size)
- **Stack Protection**: Enabled for safety
- **Exception Handling**: Limited for embedded use

### Partition Scheme (16MB Flash)

Default partition layout for 16MB flash:

| Partition | Size | Purpose |
|-----------|------|---------|
| nvs       | 20KB | Non-volatile storage |
| otadata   | 8KB  | OTA update data |
| app0      | 6.5MB| Main firmware (OTA slot 1) |
| app1      | 6.5MB| Backup firmware (OTA slot 2) |
| spiffs    | 3MB  | File system |

**Note**: OTA (Over-The-Air) updates are supported but not currently implemented.

## Troubleshooting Build Issues

### Issue: PSRAM Not Detected

**Symptoms:**
- Build succeeds but displays fail to initialize
- Error: `Failed to allocate display buffer`

**Solution:**
- Verify your ESP32-S3 board has PSRAM (look for R2, R8, or R16 suffix)
- Check build flags include PSRAM configuration:
  ```ini
  -DBOARD_HAS_PSRAM
  -DCONFIG_SPIRAM_MODE_QUAD=1
  ```

### Issue: Library Not Found

**Symptoms:**
- Build error: `dependency 'lvgl' not found`

**Solution:**
```bash
# Update PlatformIO library manager
pio pkg update

# Clean and rebuild
pio run --target clean
pio run
```

### Issue: Upload Fails

**Symptoms:**
- `Failed to connect to ESP32-S3`

**Solution:**
- Press and hold BOOT button on ESP32-S3
- Press RESET button once
- Release BOOT button
- Try upload again
- Or reduce upload speed: `upload_speed = 460800`

### Issue: Serial Monitor Shows Garbage

**Symptoms:**
- Random characters in serial output

**Solution:**
- Ensure monitor speed matches firmware: `115200` baud
- Check USB cable quality (some cables are power-only)
- Try different USB port

### Issue: Insufficient Flash

**Symptoms:**
- Build error: `section '.flash.rodata' will not fit in region 'drom0_0_seg'`

**Solution:**
- Remove unused apps from `main.cpp`
- Optimize assets (compress images, reduce size)
- Use SPIFFS for large assets instead of embedding

## Custom Configuration

### Changing Display Resolution

If using different displays (e.g., 320x240):

```ini
-DDISPLAY_WIDTH=320
-DDISPLAY_HEIGHT=240
```

Update in both `platformio.ini` and [src/main.cpp](../src/main.cpp):
```cpp
#define TFT_WIDTH 320
#define TFT_HEIGHT 240
```

### Changing WiFi Credentials

Edit [src/main.cpp](../src/main.cpp):
```cpp
const char* STATION_SSID = "YourWiFi";
const char* STATION_PASSWORD = "YourPassword";
const char* AP_SSID = "DokiOS-Control";
const char* AP_PASSWORD = "doki1234";
```

### Changing API Keys

Edit [src/main.cpp](../src/main.cpp):
```cpp
const char* WEATHER_API_KEY = "your_api_key_here";
```

Get a free API key from: https://www.weatherapi.com/

## Advanced Configuration

### Enabling Debug Logging

Add to build flags:
```ini
-DCORE_DEBUG_LEVEL=5
```

Levels: 0=None, 1=Error, 2=Warn, 3=Info, 4=Debug, 5=Verbose

### Changing CPU Frequency

For lower power consumption:
```ini
board_build.f_cpu = 160000000L  ; 160MHz
```

**Note**: May affect display refresh rate and performance.

### Disabling Animations

Edit [include/lv_conf.h](../include/lv_conf.h):
```c
#define LV_USE_ANIMATION 0
```

Reduces CPU usage but removes smooth transitions.

## Next Steps

After configuring the platform:
1. Proceed to [System Architecture](./03-system-architecture.md) to understand the codebase
2. Build and upload the firmware
3. Test with default apps

---

[← Back: Hardware Setup](./01-hardware-setup.md) | [Next: System Architecture →](./03-system-architecture.md)
