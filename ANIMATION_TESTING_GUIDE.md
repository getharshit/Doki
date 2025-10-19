# Animation System Testing Guide

## Overview
This guide walks you through testing the complete animation system on your ESP32-S3 hardware.

---

## Prerequisites

✅ **All Completed:**
- Animation system code compiled successfully
- Firmware uploaded to ESP32-S3
- SPIFFS filesystem uploaded with:
  - `/animations/test.spr` (64×64, 20-frame spinner)
  - `/apps/animation_test.js` (test application)

---

## Step 1: Connect and Monitor Serial

### Action
1. Open PlatformIO Serial Monitor
2. Press **RESET** button on ESP32-S3
3. Watch boot sequence

### Expected Output
```
╔═══════════════════════════════════════════════════╗
║         Doki OS - Refactored Architecture         ║
║                  Version 0.2.0                     ║
╚═══════════════════════════════════════════════════╝

[Main] Step 1/6: Initializing storage...
[Main] ✓ Storage initialized

[Main] Step 1.5/6: Initializing filesystem...
[FilesystemManager] ✓ SPIFFS initialized

[Main] Step 3.6/8: Initializing JavaScript Engine...
[JSEngine] ✓ Duktape context created
[JSEngine] ✓ Registered Doki OS APIs (Advanced Features Enabled + Animation)

[Main] ✓ Setup complete!
```

### Verify
- ✅ SPIFFS initialized successfully
- ✅ JavaScript Engine initialized
- ✅ Animation APIs registered
- ✅ No errors during boot

---

## Step 2: Get System IP Address

### From Serial Monitor
Look for the line:
```
📱 Dashboard: http://192.168.X.X
🌐 IP Address: 192.168.X.X
```

### Note the IP Address
Example: `192.168.1.100`

---

## Step 3: Open Dashboard

### Action
Open web browser and navigate to:
```
http://192.168.X.X/
```
(Replace X.X with your actual IP)

### Expected
- Doki OS dashboard loads
- Shows 2 displays (Display 0 and Display 1)
- Lists available apps

---

## Step 4: Load Animation Test App

### Method 1: Via HTTP API (Recommended)

Open browser and navigate to:
```
http://192.168.X.X/api/load_app?display=0&app=custom
```

Then POST JavaScript code to custom app endpoint:
```bash
curl -X POST http://10.104.170.213/api/custom_code/0 \
  -H "Content-Type: text/plain" \
  --data-binary @data/apps/animation_test.js
```

### Method 2: Via Dashboard
1. Click on **Display 0** or **Display 1**
2. Select **"Custom JS"** app from dropdown
3. Click **"Load App"**
4. Upload `animation_test.js` via the code editor

### Expected Serial Output
```
[AppManager] Loading app 'custom' on display 0
[JSApp] Creating JSApp: custom
[JSApp] ✓ JSApp created successfully
[JSEngine] Executing JS code (XXX bytes)
[JSEngine] Calling onCreate()
Animation Test App - Starting
Animation loaded successfully: ID=0
Animation playing
```

---

## Step 5: Observe Display

### What to Look For

**On Physical Display:**
1. **Background**: Black screen
2. **Text Labels**:
   - "Animation Test" (top)
   - "Playing (loop)" (center)
   - "Speed: 1.0x"
   - "Opacity: 255"
3. **Animation**: 64×64 spinning circle/spinner
   - Should be smoothly animating at 30 FPS
   - Positioned at center of screen

### Animation Behavior Timeline

| Time | Action | What You See |
|------|--------|--------------|
| 0s | Load & Play | Spinner starts rotating (loop mode) |
| 3s | Speed change | Spinner rotates 2x faster |
| 6s | Opacity change | Spinner becomes semi-transparent |
| 9s | Pause | Spinner freezes at current frame |
| 12s | Resume | Spinner continues from frozen position |
| 15s | Reset | Back to normal speed and opacity |
| 18s | Stop | Spinner stops and resets to frame 0 |
| 21s | Replay | Cycle repeats |

---

## Step 6: Check Serial Monitor

### Expected Log Output

```
[JSEngine] Calling onUpdate()
Animation Test App - Starting
Animation loaded successfully: ID=0
Animation playing

// After 3 seconds
Testing speed: 2.0x

// After 6 seconds
Testing opacity: 128

// After 9 seconds
Testing pause

// After 12 seconds
Testing resume

// After 15 seconds
Reset to normal

// After 18 seconds
Testing stop

// After 21 seconds
Testing replay
```

---

## Step 7: Performance Check

### Monitor FPS and Memory

Check serial output for:
```
[AnimationManager] Update: 1 animations active
[AnimationManager] Memory used: 314KB / 1024KB
[AnimationManager] Frame rate: 30 FPS
```

### Expected Performance
- **Frame Rate**: Steady 30 FPS
- **Memory Usage**: ~314KB (64×64, 20 frames, 8-bit indexed)
- **CPU Usage**: ~5% (8-bit to RGB565 conversion)
- **No lag or stuttering**

---

## Troubleshooting

### Problem: "Animation load failed"

**Serial Shows:**
```
Failed to load animation
```

**Causes & Fixes:**
1. **Sprite file missing**
   - Verify: `ls data/animations/test.spr`
   - Fix: Regenerate with `python3 tools/sprite_converter.py`
   - Re-upload SPIFFS: `platformio run --target uploadfs`

2. **Incorrect path**
   - Animation test app uses: `/animations/test.spr`
   - Check SPIFFS path matches

3. **Corrupted sprite file**
   - Check magic number: Should be 0x444F4B49 ("DOKI")
   - Regenerate sprite file

---

### Problem: Animation won't play

**Serial Shows:**
```
Animation loaded successfully: ID=0
Failed to start animation
```

**Causes & Fixes:**
1. **PSRAM allocation failed**
   - Check available PSRAM: ESP.getPsramSize()
   - Reduce animation size or frame count

2. **Invalid sprite data**
   - Verify sprite file integrity
   - Check frame dimensions match header

---

### Problem: Animation is choppy/stuttering

**Causes & Fixes:**
1. **Main loop too slow**
   - Check `onUpdate()` execution time
   - Must complete in < 33ms for 30 FPS

2. **SPI bus congestion**
   - Reduce SPI frequency if needed
   - Currently: 40 MHz

3. **PSRAM bandwidth**
   - QSPI PSRAM mode confirmed in platformio.ini
   - Check PSRAM clock settings

---

### Problem: "updateAnimations() not being called"

**Symptoms:**
- Animation loads but doesn't move
- Frame stays at 0

**Fix:**
Verify `onUpdate()` function exists and calls `updateAnimations()`:
```javascript
function onUpdate() {
    updateAnimations();
}
```

---

### Problem: No display output

**Causes & Fixes:**
1. **App not loaded**
   - Check AppManager status in serial
   - Verify HTTP API response

2. **LVGL not updating**
   - Main loop must call `lv_timer_handler()`
   - Check for mutex deadlock

3. **Display initialization failed**
   - Check ST7789 initialization
   - Verify SPI pins correct

---

## Advanced Testing

### Test Multiple Animations

Create a new test app that loads 2 animations simultaneously:

```javascript
var anim1 = loadAnimation("/animations/test.spr");
var anim2 = loadAnimation("/animations/test.spr");

setAnimationPosition(anim1, 50, 100);
setAnimationPosition(anim2, 150, 100);

playAnimation(anim1, true);
playAnimation(anim2, true);

function onUpdate() {
    updateAnimations();
}
```

**Expected:**
- Both animations play simultaneously
- Memory usage: ~628KB
- Still 30 FPS

---

### Test Memory Limits

Load animations until memory exhausted:

```javascript
var anims = [];

function onCreate() {
    for (var i = 0; i < 10; i++) {
        var id = loadAnimation("/animations/test.spr");
        if (id < 0) {
            log("Memory limit reached at " + i + " animations");
            break;
        }
        anims.push(id);
    }
}
```

**Expected:**
- Loads ~3 animations (314KB each)
- 4th load fails due to 1MB limit
- LRU eviction should kick in

---

### Test Different Sprite Sizes

Generate different test sprites:

**Small (32×32, 30 frames):**
```bash
python3 tools/sprite_converter.py test data/animations/small.spr \
  --pattern spinner --width 32 --height 32 --frames 30 --fps 30
```
Memory: ~30KB

**Large (128×128, 20 frames):**
```bash
python3 tools/sprite_converter.py test data/animations/large.spr \
  --pattern spinner --width 128 --height 128 --frames 20 --fps 30
```
Memory: ~327KB

**Test loading different sizes**

---

## Performance Benchmarks

### Target Metrics

| Metric | Target | Acceptable | Poor |
|--------|--------|------------|------|
| Frame Rate | 30 FPS | 25-30 FPS | < 25 FPS |
| Memory Usage | < 800KB | < 1000KB | > 1000KB |
| Load Time | < 500ms | < 1000ms | > 1000ms |
| CPU Usage | < 10% | < 15% | > 20% |

### Measure Frame Rate

Add FPS counter to test app:

```javascript
var frameCount = 0;
var lastTime = 0;

function onUpdate() {
    updateAnimations();

    frameCount++;
    var now = millis();
    if (now - lastTime > 1000) {
        var fps = frameCount;
        log("FPS: " + fps);
        frameCount = 0;
        lastTime = now;
    }
}
```

---

## Validation Checklist

Use this checklist to confirm full system functionality:

### Basic Functionality
- [ ] Sprite file loads without errors
- [ ] Animation plays on display
- [ ] Animation loops correctly
- [ ] Frame timing is smooth (30 FPS)
- [ ] No memory leaks after 5 minutes

### Playback Control
- [ ] Play starts animation
- [ ] Stop resets to frame 0
- [ ] Pause freezes at current frame
- [ ] Resume continues from pause
- [ ] Loop mode works (ONCE vs LOOP)

### Visual Controls
- [ ] Position changes work (setAnimationPosition)
- [ ] Speed changes work (0.5x, 1.0x, 2.0x)
- [ ] Opacity changes work (0-255)
- [ ] Changes take effect immediately

### Memory Management
- [ ] Animation unloads properly
- [ ] Memory freed after unload
- [ ] LRU eviction works when limit reached
- [ ] Multiple animations load/unload cleanly

### Error Handling
- [ ] Invalid filepath returns -1
- [ ] Invalid animation ID handled gracefully
- [ ] Out-of-memory handled without crash
- [ ] Corrupted sprite detected

---

## Success Criteria

✅ **System is working if:**
1. Test sprite loads successfully
2. Animation plays smoothly at 30 FPS
3. All playback controls work (play/pause/stop)
4. Speed and opacity controls work
5. No crashes or errors in serial output
6. Memory usage stays under 1MB
7. System runs for 5+ minutes without issues

---

## Next Steps After Successful Test

### Phase 4: Network Integration
1. Download sprite from web server
2. HTTP endpoint for sprite upload
3. Remote sprite library

### Phase 5: Multi-Display Support
1. Load different animations on each display
2. Synchronized playback
3. Mirror mode

### Additional Features
1. Animation callbacks (onFrame, onComplete)
2. Animation chains (sequence playback)
3. Hardware acceleration (DMA)
4. Sprite compression (RLE, LZ4)

---

## Quick Command Reference

### Generate New Sprites
```bash
# Spinner (64×64, 20 frames, 30 FPS)
python3 tools/sprite_converter.py test data/animations/spinner.spr \
  --pattern spinner --width 64 --height 64 --frames 20 --fps 30

# Bouncing ball
python3 tools/sprite_converter.py test data/animations/bounce.spr \
  --pattern bounce --width 64 --height 64 --frames 30 --fps 30

# From GIF file
python3 tools/sprite_converter.py animation.gif data/animations/custom.spr \
  --fps 30
```

### Upload to ESP32
```bash
# Upload firmware
platformio run --target upload

# Upload SPIFFS
platformio run --target uploadfs

# Monitor serial
platformio device monitor
```

### Load Test App via API
```bash
# Get IP address
DOKI_IP="192.168.1.100"

# Load custom JS app on display 0
curl "http://$DOKI_IP/api/load_app?display=0&app=custom"

# Upload JavaScript code
curl -X POST "http://$DOKI_IP/api/custom_code/0" \
  -H "Content-Type: text/plain" \
  --data-binary @data/apps/animation_test.js
```

---

## Support

### Debug Mode
Enable verbose logging by adding to animation_test.js:
```javascript
function onUpdate() {
    updateAnimations();

    // Log every 30 frames (1 second @ 30 FPS)
    if (frameCount % 30 === 0) {
        log("Frame: " + frameCount + ", Phase: " + testPhase);
    }
}
```

### Serial Commands
Monitor serial output:
```bash
platformio device monitor --baud 115200
```

### Memory Analysis
Add memory monitoring:
```javascript
function onCreate() {
    log("Free heap: " + ESP.getFreeHeap());
    log("PSRAM size: " + ESP.getPsramSize());
    log("Free PSRAM: " + ESP.getFreePsram());
}
```

---

**Ready to test!** Follow the steps above and report any issues. Good luck! 🚀
