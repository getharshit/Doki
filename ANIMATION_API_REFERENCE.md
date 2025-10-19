# Doki OS Animation API Reference

## Overview

The Animation API allows JavaScript apps to load and display animated sprites (.spr files) on Doki OS displays. All sprite data is stored in PSRAM for efficient memory usage.

## Quick Start

```javascript
// Load animation
var anim = loadAnimation("/animations/loading.spr");

// Play with loop
playAnimation(anim, true);

// Position on screen
setAnimationPosition(anim, 120, 160);

// Update in main loop
function onUpdate() {
    updateAnimations();
}

// Cleanup
function onDestroy() {
    unloadAnimation(anim);
}
```

---

## API Functions

### loadAnimation(filepath)

Load animation sprite from file.

**Parameters:**
- `filepath` (string): Path to .spr file (e.g., "/animations/loading.spr")

**Returns:**
- `number`: Animation ID (>= 0) on success, -1 on failure

**Example:**
```javascript
var animId = loadAnimation("/animations/spinner.spr");
if (animId >= 0) {
    log("Animation loaded successfully");
}
```

**Notes:**
- Maximum 2 concurrent animations playing (configurable)
- Total 1MB PSRAM pool for all animations
- LRU eviction if memory limit exceeded

---

### playAnimation(animId, loop)

Start animation playback.

**Parameters:**
- `animId` (number): Animation ID from loadAnimation()
- `loop` (boolean): true = loop forever, false = play once

**Returns:**
- `boolean`: true if started successfully, false on error

**Example:**
```javascript
// Play once
playAnimation(animId, false);

// Loop forever
playAnimation(animId, true);
```

**Notes:**
- Animation starts from frame 0
- Respects FPS setting from sprite file
- Can change loop mode by calling again

---

### stopAnimation(animId)

Stop animation playback and reset to frame 0.

**Parameters:**
- `animId` (number): Animation ID

**Returns:**
- None

**Example:**
```javascript
stopAnimation(animId);
```

**Notes:**
- Animation remains loaded in memory
- Display resets to first frame

---

### pauseAnimation(animId)

Pause animation at current frame.

**Parameters:**
- `animId` (number): Animation ID

**Returns:**
- None

**Example:**
```javascript
pauseAnimation(animId);
// Resume later with resumeAnimation()
```

---

### resumeAnimation(animId)

Resume paused animation from current frame.

**Parameters:**
- `animId` (number): Animation ID

**Returns:**
- None

**Example:**
```javascript
pauseAnimation(animId);
// ... later ...
resumeAnimation(animId);
```

---

### setAnimationPosition(animId, x, y)

Set animation position on screen.

**Parameters:**
- `animId` (number): Animation ID
- `x` (number): X coordinate (pixels)
- `y` (number): Y coordinate (pixels)

**Returns:**
- None

**Example:**
```javascript
// Center on 240x320 screen
setAnimationPosition(animId, 120, 160);

// Top-left corner
setAnimationPosition(animId, 0, 0);
```

**Notes:**
- Position is top-left corner of animation
- Updates immediately (even while playing)

---

### setAnimationSpeed(animId, speed)

Set playback speed multiplier.

**Parameters:**
- `animId` (number): Animation ID
- `speed` (number): Speed multiplier (0.1 - 10.0)
  - 1.0 = normal speed
  - 2.0 = double speed
  - 0.5 = half speed

**Returns:**
- None

**Example:**
```javascript
// Double speed
setAnimationSpeed(animId, 2.0);

// Slow motion
setAnimationSpeed(animId, 0.5);

// Back to normal
setAnimationSpeed(animId, 1.0);
```

**Notes:**
- Clamped to 0.1 - 10.0 range
- Changes take effect immediately

---

### setAnimationOpacity(animId, opacity)

Set animation opacity (transparency).

**Parameters:**
- `animId` (number): Animation ID
- `opacity` (number): Opacity value (0 - 255)
  - 255 = fully opaque
  - 128 = 50% transparent
  - 0 = fully transparent (invisible)

**Returns:**
- None

**Example:**
```javascript
// Fully opaque
setAnimationOpacity(animId, 255);

// Semi-transparent
setAnimationOpacity(animId, 128);

// Fade out effect
for (var i = 255; i >= 0; i -= 5) {
    setAnimationOpacity(animId, i);
}
```

---

### unloadAnimation(animId)

Unload animation and free memory.

**Parameters:**
- `animId` (number): Animation ID

**Returns:**
- None

**Example:**
```javascript
function onDestroy() {
    unloadAnimation(animId);
}
```

**Notes:**
- Always unload animations in onDestroy()
- Frees PSRAM memory immediately
- Animation ID becomes invalid

---

### updateAnimations()

Update all playing animations (advance frames).

**Parameters:**
- None

**Returns:**
- None

**Example:**
```javascript
function onUpdate() {
    updateAnimations();
}
```

**Notes:**
- **MUST** be called in onUpdate() for animations to play
- Updates frame timing for all animations
- Renders new frames to screen

---

## Sprite File Format (.spr)

### Header Structure (64 bytes)

```
Offset  Size  Field           Description
------  ----  -----           -----------
0x00    4     magic           Magic number: 0x444F4B49 ("DOKI")
0x04    2     version         Format version (1)
0x06    2     frameCount      Number of frames
0x08    2     frameWidth      Width in pixels
0x0A    2     frameHeight     Height in pixels
0x0C    1     fps             Frames per second
0x0D    1     colorFormat     0=8-bit indexed, 1=RGB565, 2=RGB888
0x0E    1     compression     0=none, 1=RLE, 2=LZ4
0x0F    49    reserved        Reserved for future use
```

### Data Structure

```
[Header: 64 bytes]
[Palette: 256 x 4 bytes RGBA] (if 8-bit indexed)
[Frame Offsets: frameCount x 4 bytes]
[Frame Data: variable size per frame]
```

### Memory Requirements

**8-bit Indexed Color** (recommended):
```
Memory = (width × height × 1 byte × frameCount) + 1KB palette
```

**RGB565**:
```
Memory = (width × height × 2 bytes × frameCount)
```

**Example**: 100x100 sprite, 30 frames, 8-bit indexed
```
Memory = (100 × 100 × 1 × 30) + 1024
       = 300KB + 1KB
       = 301KB
```

---

## Best Practices

### 1. Always Unload Animations

```javascript
var animId = -1;

function onCreate() {
    animId = loadAnimation("/animations/test.spr");
}

function onDestroy() {
    if (animId >= 0) {
        unloadAnimation(animId);
    }
}
```

### 2. Check Load Success

```javascript
var animId = loadAnimation("/animations/test.spr");
if (animId < 0) {
    log("Failed to load animation");
    return;
}
```

### 3. Call updateAnimations() in Loop

```javascript
function onUpdate() {
    // Required for animations to play
    updateAnimations();
}
```

### 4. Stop Before Unload

```javascript
function onDestroy() {
    if (animId >= 0) {
        stopAnimation(animId);
        unloadAnimation(animId);
    }
}
```

### 5. Use 8-bit Indexed Color

- 50% memory savings vs RGB565
- Same visual quality with 256 colors
- Faster loading times

---

## Memory Management

### Pool Size
- Total: 1MB PSRAM (configurable in hardware_config.h)
- Shared across all animations

### LRU Eviction
- Automatic eviction of least recently used animations
- Prevents out-of-memory errors
- Pin important animations to prevent eviction

### Concurrent Animations
- Default: 2 simultaneous playing animations
- Configurable in hardware_config.h: `MAX_CONCURRENT_ANIMATIONS`

---

## Performance Guidelines

### Frame Rates
- **30 FPS**: Default, smooth for most animations
- **60 FPS**: Very smooth, higher CPU usage
- **24 FPS**: Cinematic feel, lower CPU usage
- **15 FPS**: Low power mode

### Sprite Size Recommendations

| Size       | Frame Count | Format        | Memory   |
|------------|-------------|---------------|----------|
| 64x64      | 60          | 8-bit indexed | 240KB    |
| 100x100    | 30          | 8-bit indexed | 300KB    |
| 128x128    | 20          | 8-bit indexed | 320KB    |
| 200x200    | 10          | 8-bit indexed | 400KB    |

### CPU Usage
- 8-bit indexed: ~5% CPU (conversion overhead)
- RGB565: ~2% CPU (direct copy)
- Trade-off: Memory vs CPU

---

## Troubleshooting

### Animation Won't Load

**Check:**
1. File exists in SPIFFS: `/animations/filename.spr`
2. Magic number is correct: 0x444F4B49
3. Sufficient PSRAM available
4. Valid sprite format

**Debug:**
```javascript
var animId = loadAnimation("/animations/test.spr");
if (animId < 0) {
    log("Load failed - check serial output");
}
```

### Animation Won't Play

**Check:**
1. Called `playAnimation()` after load
2. Calling `updateAnimations()` in `onUpdate()`
3. Animation not paused or stopped

**Debug:**
```javascript
var success = playAnimation(animId, true);
if (!success) {
    log("Play failed - invalid animation ID?");
}
```

### Animation Choppy

**Solutions:**
1. Reduce frame rate (use 24 FPS instead of 60 FPS)
2. Reduce sprite size
3. Use 8-bit indexed color
4. Ensure `onUpdate()` runs quickly (< 33ms)

### Out of Memory

**Solutions:**
1. Unload unused animations
2. Use smaller sprites
3. Reduce frame counts
4. Increase pool size in hardware_config.h

---

## Example: Loading Screen

```javascript
var loadingAnim = -1;

function onCreate() {
    setBackgroundColor(0x000000);

    // Create loading text
    createLabel("Loading...", 120, 100);

    // Load spinner animation
    loadingAnim = loadAnimation("/animations/spinner.spr");

    if (loadingAnim >= 0) {
        setAnimationPosition(loadingAnim, 100, 140);
        playAnimation(loadingAnim, true); // Loop forever
    }
}

function onUpdate() {
    updateAnimations();

    // Simulate loading progress
    // When done, unload animation and transition
}

function onDestroy() {
    if (loadingAnim >= 0) {
        stopAnimation(loadingAnim);
        unloadAnimation(loadingAnim);
    }
}
```

---

## Example: Button Press Animation

```javascript
var buttonAnim = -1;

function onCreate() {
    // Load button press animation (plays once)
    buttonAnim = loadAnimation("/animations/button_press.spr");

    if (buttonAnim >= 0) {
        setAnimationPosition(buttonAnim, 100, 200);
        // Don't play yet - wait for button press
    }
}

function onButtonPress() {
    if (buttonAnim >= 0) {
        playAnimation(buttonAnim, false); // Play once
    }
}

function onUpdate() {
    updateAnimations();
}

function onDestroy() {
    if (buttonAnim >= 0) {
        unloadAnimation(buttonAnim);
    }
}
```

---

## System Configuration

### hardware_config.h

```cpp
// Animation memory pool
#define ANIMATION_POOL_SIZE_KB          1024    // 1MB total

// Frame buffer per animation
#define ANIMATION_FRAME_BUFFER_SIZE_KB  512     // 512KB

// Maximum concurrent animations
#define MAX_CONCURRENT_ANIMATIONS       2

// Metadata cache
#define ANIMATION_CACHE_SIZE_KB         200     // 200KB
```

### timing_constants.h

```cpp
// Frame rates
#define UPDATE_INTERVAL_ANIMATION_MS    33      // 30 FPS
#define UPDATE_INTERVAL_ANIMATION_60FPS_MS 16   // 60 FPS
#define UPDATE_INTERVAL_ANIMATION_24FPS_MS 42   // 24 FPS
#define UPDATE_INTERVAL_ANIMATION_15FPS_MS 67   // 15 FPS

// Timeouts
#define TIMEOUT_ANIMATION_LOAD_MS       5000    // 5 seconds
#define TIMEOUT_ANIMATION_DOWNLOAD_MS   30000   // 30 seconds
```

---

## Future Features (Planned)

### Phase 4: Network Integration
- Download sprites from web server
- Streaming animations
- Remote animation library

### Phase 5: Multi-Display Support
- Sync animations across displays
- Mirror animations
- Display-specific positioning

### Additional Features
- Sprite callbacks (onFrame, onComplete)
- Animation chains (play sequence)
- Sprite compression (RLE, LZ4)
- Hardware acceleration (DMA)

---

## Summary

The Animation API provides a complete system for displaying animated sprites in JavaScript apps with:

- ✅ Simple 10-function API
- ✅ Efficient PSRAM storage
- ✅ Automatic memory management (LRU eviction)
- ✅ Flexible playback control (play/pause/stop)
- ✅ Speed and opacity control
- ✅ Multiple loop modes
- ✅ 8-bit indexed color (50% memory savings)

**Start with**: Load → Position → Play → Update in loop → Unload
