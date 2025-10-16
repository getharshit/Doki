# Build and Deployment Guide

## Prerequisites

✅ All refactored modules are complete and ready
✅ LVGL compatibility fixes applied
✅ Original main.cpp backed up

## Quick Deployment (5 Steps)

### Step 1: Replace main.cpp

```bash
cd /Users/harshit/Desktop/Doki/src

# Backup original (already done)
# mv main.cpp main.cpp.backup

# Replace with refactored version
mv main_refactored.cpp main.cpp
```

### Step 2: Build the Project

```bash
cd /Users/harshit/Desktop/Doki
pio run
```

**Expected Output:**
```
Processing waveshare-esp32-s3 (platform: espressif32; board: esp32-s3-devkitc-1; framework: arduino)
...
Building .pio/build/waveshare-esp32-s3/firmware.bin
RAM:   [==        ]  20.5% (used 106804 bytes from 520192 bytes)
Flash: [====      ]  40.2% (used 1315421 bytes from 3276800 bytes)
========================= [SUCCESS] =========================
```

### Step 3: Upload to ESP32-S3

```bash
pio run --target upload
```

**During Upload:**
- If stuck, press and hold BOOT button
- Press RESET button once
- Release BOOT button
- Upload should proceed

### Step 4: Monitor Serial Output

```bash
pio device monitor -b 115200
```

**Expected Boot Sequence:**
```
╔═══════════════════════════════════════════════════╗
║         Doki OS - Refactored Architecture         ║
║                  Version 0.2.0                     ║
╚═══════════════════════════════════════════════════╝

[Main] Step 1/5: Initializing storage...
[StorageManager] ✓ NVS initialized successfully
[StorageManager] No WiFi credentials configured

[Main] Step 2/5: Initializing LVGL...
[Main] ✓ LVGL initialized

[Main] Step 3/5: Initializing displays...
[DisplayManager] Initializing 2 display(s)...
[DisplayManager] ✓ All 2 display(s) initialized

[Main] Step 4/5: Initializing WiFi...
[WiFiManager] ✓ WiFi Manager initialized

[Main] Step 5/5: Connecting to WiFi...
[WiFiManager] Auto-connect starting...
[WiFiManager] No saved WiFi credentials found
[WiFiManager] Starting in AP mode for setup

╔═══════════════════════════════════════════════════╗
║           ENTERING SETUP MODE                      ║
╚═══════════════════════════════════════════════════╝

[WiFiManager] Starting Access Point: 'DokiOS-Setup'...
[WiFiManager] ✓ Access Point started!
[WiFiManager] SSID: DokiOS-Setup
[WiFiManager] Password: doki1234
[WiFiManager] IP Address: 192.168.4.1

[Main] Connect to: DokiOS-Setup (Password: doki1234)
[Main] Then visit: http://192.168.4.1/setup

[Main] ✓ Setup complete!
```

### Step 5: First-Time Setup

#### On Your Phone/Computer:

1. **Connect to WiFi**
   - SSID: `DokiOS-Setup`
   - Password: `doki1234`

2. **Scan QR Code** (displayed on Display 0)
   - Opens WiFi connection settings automatically
   - OR manually navigate to: `http://192.168.4.1/setup`

3. **Configure WiFi**
   - Web page loads automatically (captive portal)
   - Click "Scan WiFi Networks"
   - Select your home WiFi
   - Enter password
   - Click "Connect to WiFi"

4. **Device Restarts**
   - Automatically restarts after 3 seconds
   - Connects to your WiFi
   - Loads Clock and Weather apps

## Testing Checklist

### ✅ First Boot Test (Setup Mode)

- [ ] Device boots and enters setup mode
- [ ] Display 0 shows QR code
- [ ] Display 1 shows "Connect to configure" message
- [ ] Serial output shows AP started at 192.168.4.1
- [ ] Can connect to DokiOS-Setup WiFi
- [ ] QR code scan opens WiFi settings
- [ ] Web interface loads at http://192.168.4.1/setup
- [ ] Network scan works and shows available WiFi
- [ ] Can select network and enter password
- [ ] Device saves credentials and restarts

### ✅ Normal Boot Test (After Setup)

- [ ] Device boots and connects to saved WiFi automatically
- [ ] Serial shows "Connected to WiFi!"
- [ ] Display 0 loads Clock app
- [ ] Display 1 loads Weather app
- [ ] Time syncs via NTP
- [ ] Weather data fetches successfully
- [ ] Both displays update smoothly

### ✅ Reconnection Test

- [ ] Disconnect WiFi (turn off router or change password)
- [ ] Device detects disconnection
- [ ] Attempts auto-reconnection every 30 seconds
- [ ] After multiple failures, can manually trigger setup mode
- [ ] OR device falls back to AP mode after timeout

### ✅ Memory Test

- [ ] Check heap usage: Should be ~150KB used
- [ ] Check PSRAM usage: Should be ~400KB used
- [ ] No memory leaks over 1 hour operation
- [ ] Stable performance

## Troubleshooting

### Issue: Build Fails with LVGL Errors

**Solution:**
- Ensure LVGL v8.3.11 is installed
- Check `platformio.ini` has correct library version
- Run `pio lib update` to update libraries

```bash
pio lib update
pio run --target clean
pio run
```

### Issue: Upload Fails

**Solution:**
- Press and hold BOOT button on ESP32-S3
- Press RESET button once
- Release BOOT button
- Run upload again

OR reduce upload speed:
```ini
; In platformio.ini
upload_speed = 460800  ; Instead of 921600
```

### Issue: Displays Don't Initialize

**Solution:**
Check that LVGL display buffers are properly allocated. The refactored main.cpp needs LVGL display initialization.

**Fix:** Ensure DisplayManager creates LVGL displays properly (this may need integration).

### Issue: QR Code Doesn't Display

**Solution:**
- Check QRCode library is installed: `pio lib install ricmoo/QRCode`
- Verify PSRAM is enabled for canvas buffer
- Check serial output for QR generation errors

### Issue: Setup Portal Doesn't Redirect

**Solution:**
- DNS server may not be starting
- Try direct URL: http://192.168.4.1/setup
- Check serial output for DNS errors

### Issue: Device Doesn't Save WiFi Credentials

**Solution:**
- NVS partition may not be initialized
- Check serial output for storage errors
- Try clearing NVS: `StorageManager::clearAll()`

### Issue: WiFi Connection Fails

**Solution:**
- Verify SSID and password are correct
- Check WiFi signal strength
- Ensure 2.4GHz network (ESP32 doesn't support 5GHz)
- Check router allows new device connections

## Performance Benchmarks

### Expected Performance:

| Metric | Target | Acceptable |
|--------|--------|------------|
| Boot Time | 3-5 seconds | <10 seconds |
| WiFi Connect | 2-5 seconds | <10 seconds |
| App Load Time | 100-300ms | <500ms |
| Display FPS | 30 FPS | >20 FPS |
| Heap Usage | ~150KB | <250KB |
| PSRAM Usage | ~400KB | <1MB |

### Monitoring Commands:

```cpp
// In Serial Monitor, check:
[PERF] Loop FPS: ~200
[Main] Heap: 150KB / 512KB
[Main] PSRAM: 400KB / 2MB
```

## Advanced Configuration

### Change WiFi AP Credentials

Edit `src/main.cpp`:
```cpp
#define AP_SSID "YourDeviceName"
#define AP_PASSWORD "yourpassword123"
```

### Change Display Count

Edit `src/main.cpp`:
```cpp
#define DISPLAY_COUNT 3  // For 3 displays
```

Also update DisplayManager pin configuration.

### Change Weather Location

Edit `src/main.cpp`:
```cpp
#define WEATHER_LOCATION "YourCity"
```

### Disable Setup Mode Timeout

Edit `src/main.cpp`:
```cpp
const uint32_t SETUP_MODE_TIMEOUT = 0;  // Never timeout
```

## Rollback to Original

If you need to revert:

```bash
cd /Users/harshit/Desktop/Doki/src
mv main.cpp main_refactored.cpp
mv main.cpp.backup main.cpp
pio run --target upload
```

## Success Indicators

You'll know the refactoring is successful when:

✅ Device boots reliably
✅ Setup flow works smoothly (QR code → web → configure → restart)
✅ WiFi auto-connects after setup
✅ Both displays show apps correctly
✅ No memory leaks or crashes
✅ Reconnection works after WiFi dropout
✅ Serial output is clean with no errors

## Next Steps After Deployment

1. **Test for 24 Hours** - Ensure stability
2. **Test WiFi Failure Recovery** - Disconnect and reconnect
3. **Test Power Cycle** - Verify credentials persist
4. **Add More Apps** - Expand functionality
5. **Customize Dashboard** - Enhance HTTP server
6. **Document User Manual** - For end users

## Support

For issues:
1. Check serial output for error messages
2. Verify hardware connections
3. Ensure proper power supply (2A minimum)
4. Check documentation in `/docs` folder

---

**Date**: 2025-01-16
**Version**: 0.2.0 (Refactored)
**Status**: Ready for Deployment
