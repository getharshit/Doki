# Doki OS - Sprite File Format Specification (.spr)

**Version:** 1.0
**Last Updated:** 2025-01-19
**Status:** Stable

Complete binary format specification for `.spr` sprite animation files used in Doki OS.

---

## Table of Contents

1. [Overview](#overview)
2. [File Structure](#file-structure)
3. [Header Format](#header-format)
4. [Data Sections](#data-sections)
5. [Color Formats](#color-formats)
6. [Example Files](#example-files)
7. [Validation](#validation)
8. [Common Mistakes](#common-mistakes)
9. [Reference Implementation](#reference-implementation)

---

## Overview

The `.spr` format is a simple, uncompressed sprite sheet format optimized for embedded systems with limited RAM. All multi-byte values use **little-endian** byte order.

### Design Goals

- **Simple parsing**: Header is a fixed 64-byte struct, directly mappable to memory
- **Memory efficient**: 8-bit indexed color reduces memory by 50-75% vs RGB565
- **Fast rendering**: Pre-converted RGB565 palette for zero-cost color lookup
- **Extensible**: 49 bytes reserved for future features

### Key Features

✅ Fixed 64-byte header for fast parsing
✅ Support for 8-bit indexed, RGB565, and RGB888 color
✅ Optional RLE or LZ4 compression (future)
✅ Up to 120 frames per sprite
✅ Up to 240×320 pixels per frame

---

## File Structure

```
┌─────────────────────────────────────┐
│ Header (64 bytes)                   │  ← Always present
├─────────────────────────────────────┤
│ Palette (1024 bytes)                │  ← Only if 8-bit indexed color
│ 256 colors × 4 bytes RGBA           │
├─────────────────────────────────────┤
│ Frame Data (variable size)          │  ← Pixel data for all frames
│ frameCount × (width × height × bpp) │
└─────────────────────────────────────┘
```

### Size Calculation

**8-bit Indexed Color:**
```
Total Size = 64 + 1024 + (width × height × 1 × frameCount)
```

**RGB565:**
```
Total Size = 64 + (width × height × 2 × frameCount)
```

**RGB888:**
```
Total Size = 64 + (width × height × 3 × frameCount)
```

---

## Header Format

The header is exactly **64 bytes** and must be written with **little-endian** byte order.

### Header Structure

```c
struct SpriteHeader {
    uint32_t magic;                 // Magic number "DOKI" (0x444F4B49)
    uint16_t version;               // Format version (currently 1)
    uint16_t frameCount;            // Number of frames (1-120)
    uint16_t frameWidth;            // Width in pixels (1-240)
    uint16_t frameHeight;           // Height in pixels (1-320)
    uint8_t fps;                    // Frames per second (1-60)
    uint8_t colorFormat;            // Color format (0=8bit, 1=RGB565, 2=RGB888)
    uint8_t compression;            // Compression (0=none, 1=RLE, 2=LZ4)
    uint8_t reserved[49];           // Reserved for future use (set to 0)
} __attribute__((packed));
```

### Byte Layout

```
Offset  Size  Type      Field           Value/Description
------  ----  --------  --------------  ----------------------------------
0x00    4     uint32_t  magic           0x444F4B49 ("DOKI" little-endian)
0x04    2     uint16_t  version         1 (format version)
0x06    2     uint16_t  frameCount      Number of frames (1-120)
0x08    2     uint16_t  frameWidth      Width in pixels (1-240)
0x0A    2     uint16_t  frameHeight     Height in pixels (1-320)
0x0C    1     uint8_t   fps             Frames per second (1-60)
0x0D    1     uint8_t   colorFormat     0=8-bit, 1=RGB565, 2=RGB888
0x0E    1     uint8_t   compression     0=none, 1=RLE, 2=LZ4
0x0F    49    uint8_t[] reserved        Reserved (fill with zeros)
```

### Field Descriptions

#### `magic` (0x444F4B49)
- **Type:** `uint32_t` (4 bytes, little-endian)
- **Value:** `0x444F4B49` (ASCII "DOKI" backwards due to little-endian)
- **Bytes:** `49 4B 4F 44` (hex)
- **Purpose:** File format validation

#### `version` (1)
- **Type:** `uint16_t` (2 bytes, little-endian)
- **Value:** `1`
- **Bytes:** `01 00` (hex)
- **Purpose:** Format version for backward compatibility

#### `frameCount` (1-120)
- **Type:** `uint16_t` (2 bytes, little-endian)
- **Range:** 1 to 120
- **Purpose:** Number of animation frames

#### `frameWidth` (1-240)
- **Type:** `uint16_t` (2 bytes, little-endian)
- **Range:** 1 to 240
- **Purpose:** Frame width in pixels

#### `frameHeight` (1-320)
- **Type:** `uint16_t` (2 bytes, little-endian)
- **Range:** 1 to 320
- **Purpose:** Frame height in pixels

#### `fps` (1-60)
- **Type:** `uint8_t` (1 byte)
- **Range:** 1 to 60
- **Purpose:** Target playback speed

#### `colorFormat` (0, 1, or 2)
- **Type:** `uint8_t` (1 byte)
- **Values:**
  - `0` = 8-bit indexed color (256 colors, requires palette)
  - `1` = RGB565 (16-bit, 65K colors, no palette)
  - `2` = RGB888 (24-bit, 16M colors, no palette)

#### `compression` (0, 1, or 2)
- **Type:** `uint8_t` (1 byte)
- **Values:**
  - `0` = None (raw pixel data)
  - `1` = RLE (run-length encoding) - **not yet supported**
  - `2` = LZ4 (LZ4 compression) - **not yet supported**

#### `reserved` (49 bytes)
- **Type:** `uint8_t[49]`
- **Value:** All zeros
- **Purpose:** Reserved for future format extensions

---

## Data Sections

### Palette Section (8-bit Indexed Color Only)

If `colorFormat == 0`, a 1024-byte palette follows the header.

#### Structure
```
Offset       Size  Description
-----------  ----  -----------
0x40         1024  256 RGBA colors (4 bytes each)
```

#### Format
```c
struct RGBAColor {
    uint8_t r;  // Red (0-255)
    uint8_t g;  // Green (0-255)
    uint8_t b;  // Blue (0-255)
    uint8_t a;  // Alpha (0-255)
} __attribute__((packed));

RGBAColor palette[256];  // 1024 bytes total
```

#### Example
```
Color 0:   FF 00 00 FF  (Red, fully opaque)
Color 1:   00 FF 00 FF  (Green, fully opaque)
Color 2:   00 00 FF 80  (Blue, 50% transparent)
...
Color 255: 00 00 00 00  (Transparent)
```

### Frame Data Section

Pixel data for all frames, stored sequentially.

#### 8-bit Indexed Color
```
Size per frame = width × height × 1 byte
Total size = frameCount × size per frame
Format: palette index (0-255) per pixel
```

#### RGB565
```
Size per frame = width × height × 2 bytes
Total size = frameCount × size per frame
Format: 16-bit RGB565 (5 bits R, 6 bits G, 5 bits B)
Layout: RRRRRGGG GGGBBBBB (little-endian)
```

#### RGB888
```
Size per frame = width × height × 3 bytes
Total size = frameCount × size per frame
Format: 24-bit RGB (8 bits each)
Layout: RR GG BB (3 bytes per pixel)
```

---

## Color Formats

### 8-bit Indexed Color (Recommended)

**Pros:**
- Smallest file size (1/2 to 1/3 of RGB565)
- Fast rendering with pre-converted palette
- Supports transparency via alpha channel

**Cons:**
- Limited to 256 colors
- Requires palette (1024 bytes)

**Best For:**
- Icons, UI elements, simple animations
- Sprites with limited color palettes
- Memory-constrained systems

### RGB565

**Pros:**
- 65,536 colors (good color depth)
- No palette needed

**Cons:**
- 2× larger than 8-bit indexed
- No alpha channel (transparency not supported)

**Best For:**
- Photorealistic images
- Gradients and complex colors

### RGB888

**Pros:**
- 16.7 million colors (full color)
- High quality

**Cons:**
- 3× larger than 8-bit indexed
- No alpha channel
- Slower rendering

**Best For:**
- High-quality photos (rarely used on embedded)

---

## Example Files

### Example 1: Small Icon (16×16, 8-bit, 4 frames)

```
Header (64 bytes):
  Magic:       49 4B 4F 44  (0x444F4B49)
  Version:     01 00         (1)
  Frame Count: 04 00         (4 frames)
  Width:       10 00         (16 pixels)
  Height:      10 00         (16 pixels)
  FPS:         10            (10 fps)
  Color:       00            (8-bit indexed)
  Compression: 00            (none)
  Reserved:    [49 bytes of zeros]

Palette (1024 bytes):
  256 RGBA colors

Frame Data (1024 bytes):
  Frame 0: 256 bytes (16×16×1)
  Frame 1: 256 bytes
  Frame 2: 256 bytes
  Frame 3: 256 bytes

Total: 64 + 1024 + 1024 = 2112 bytes
```

### Example 2: Medium Sprite (100×100, 8-bit, 30 frames)

```
Header: 64 bytes
Palette: 1024 bytes
Frames: 30 × (100×100×1) = 300,000 bytes

Total: 64 + 1024 + 300,000 = 301,088 bytes (~294 KB)
```

### Example 3: Full Screen (240×320, RGB565, 10 frames)

```
Header: 64 bytes
Palette: 0 bytes (RGB565 has no palette)
Frames: 10 × (240×320×2) = 1,536,000 bytes

Total: 64 + 1,536,000 = 1,536,064 bytes (~1.5 MB)
```

---

## Validation

### Client-Side Validation (Before Upload)

```javascript
async function validateSpriteFile(blob: Blob): Promise<boolean> {
  // Read header (64 bytes)
  const headerBuffer = await blob.slice(0, 64).arrayBuffer()
  const view = new DataView(headerBuffer)

  // Check magic number
  const magic = view.getUint32(0, true)  // Little-endian
  if (magic !== 0x444F4B49) {
    throw new Error('Invalid magic number')
  }

  // Check version
  const version = view.getUint16(4, true)
  if (version !== 1) {
    throw new Error(`Unsupported version: ${version}`)
  }

  // Read metadata
  const frameCount = view.getUint16(6, true)
  const width = view.getUint16(8, true)
  const height = view.getUint16(10, true)
  const fps = view.getUint8(12)
  const colorFormat = view.getUint8(13)

  // Validate ranges
  if (frameCount < 1 || frameCount > 120) {
    throw new Error(`Invalid frame count: ${frameCount}`)
  }
  if (width < 1 || width > 240) {
    throw new Error(`Invalid width: ${width}`)
  }
  if (height < 1 || height > 320) {
    throw new Error(`Invalid height: ${height}`)
  }
  if (fps < 1 || fps > 60) {
    throw new Error(`Invalid FPS: ${fps}`)
  }
  if (colorFormat > 2) {
    throw new Error(`Invalid color format: ${colorFormat}`)
  }

  // Calculate expected size
  let expectedSize = 64  // Header
  let bytesPerPixel = 1

  if (colorFormat === 0) {
    expectedSize += 1024  // Palette
    bytesPerPixel = 1
  } else if (colorFormat === 1) {
    bytesPerPixel = 2
  } else if (colorFormat === 2) {
    bytesPerPixel = 3
  }

  expectedSize += frameCount * width * height * bytesPerPixel

  if (blob.size !== expectedSize) {
    throw new Error(
      `File size mismatch: got ${blob.size}, expected ${expectedSize}`
    )
  }

  return true
}
```

### Server-Side Validation (ESP32)

The server validates:
1. Minimum file size (64 bytes)
2. Magic number (0x444F4B49)
3. Version (1)
4. Dimensions (1-240 × 1-320)
5. Frame count (1-120)
6. FPS (1-60)

---

## Common Mistakes

### ❌ Mistake 1: Missing `version` and `frameCount` Fields

**Symptoms:**
```
Error: Unsupported version (got 240, expected 1)
Version field contains frameWidth value!
```

**Cause:**
Client writes:
```
0x00: magic (4 bytes)
0x04: frameWidth (2 bytes)  ← Should be version!
0x06: frameHeight (2 bytes)
```

**Fix:**
Write version and frameCount before dimensions:
```
0x00: magic (4 bytes)
0x04: version (2 bytes)      ← Add this!
0x06: frameCount (2 bytes)   ← Add this!
0x08: frameWidth (2 bytes)
0x0A: frameHeight (2 bytes)
```

### ❌ Mistake 2: Wrong Byte Order (Big-Endian)

**Symptoms:**
```
Error: Invalid magic number (got 0x444F4B49, expected 0x494B4F44)
```

**Cause:**
Writing values in big-endian instead of little-endian.

**Fix:**
Use little-endian byte order:
```javascript
// WRONG (big-endian):
view.setUint32(0, 0x444F4B49, false)

// CORRECT (little-endian):
view.setUint32(0, 0x444F4B49, true)
```

### ❌ Mistake 3: Wrong Field Types

**Symptoms:**
```
Error: Data size mismatch
```

**Cause:**
Using `uint32_t` instead of `uint16_t` for version/frameCount/dimensions.

**Fix:**
Use correct types:
```javascript
// WRONG:
view.setUint32(4, 1)  // version as 4 bytes

// CORRECT:
view.setUint16(4, 1, true)  // version as 2 bytes
```

### ❌ Mistake 4: Missing Palette for 8-bit Indexed

**Symptoms:**
```
Error: Data too small (got X, expected Y)
```

**Cause:**
Forgot to include 1024-byte palette after header for 8-bit indexed color.

**Fix:**
Always include palette for `colorFormat == 0`:
```javascript
if (colorFormat === 0) {
  // Write 256 RGBA colors (1024 bytes) after header
  for (let i = 0; i < 256; i++) {
    view.setUint8(64 + i * 4 + 0, palette[i].r)
    view.setUint8(64 + i * 4 + 1, palette[i].g)
    view.setUint8(64 + i * 4 + 2, palette[i].b)
    view.setUint8(64 + i * 4 + 3, palette[i].a)
  }
}
```

---

## Reference Implementation

### JavaScript/TypeScript Sprite Writer

```typescript
interface SpriteOptions {
  width: number
  height: number
  frameCount: number
  fps: number
  colorFormat: 0 | 1 | 2  // 0=8bit, 1=RGB565, 2=RGB888
}

function createSpriteFile(
  frames: Uint8Array[],
  palette: { r: number; g: number; b: number; a: number }[] | null,
  options: SpriteOptions
): Blob {
  const { width, height, frameCount, fps, colorFormat } = options

  // Calculate sizes
  const headerSize = 64
  const paletteSize = colorFormat === 0 ? 1024 : 0
  const bytesPerPixel = colorFormat === 0 ? 1 : colorFormat === 1 ? 2 : 3
  const frameDataSize = frameCount * width * height * bytesPerPixel
  const totalSize = headerSize + paletteSize + frameDataSize

  // Allocate buffer
  const buffer = new ArrayBuffer(totalSize)
  const view = new DataView(buffer)

  // Write header (64 bytes)
  let offset = 0

  // Magic: 0x444F4B49 ("DOKI")
  view.setUint32(offset, 0x444F4B49, true)
  offset += 4

  // Version: 1
  view.setUint16(offset, 1, true)
  offset += 2

  // Frame count
  view.setUint16(offset, frameCount, true)
  offset += 2

  // Dimensions
  view.setUint16(offset, width, true)
  offset += 2
  view.setUint16(offset, height, true)
  offset += 2

  // FPS
  view.setUint8(offset, fps)
  offset += 1

  // Color format
  view.setUint8(offset, colorFormat)
  offset += 1

  // Compression (0 = none)
  view.setUint8(offset, 0)
  offset += 1

  // Reserved (49 bytes of zeros)
  // Already zero-initialized by ArrayBuffer
  offset = 64

  // Write palette (if 8-bit indexed)
  if (colorFormat === 0 && palette) {
    for (let i = 0; i < 256; i++) {
      const color = palette[i] || { r: 0, g: 0, b: 0, a: 0 }
      view.setUint8(offset++, color.r)
      view.setUint8(offset++, color.g)
      view.setUint8(offset++, color.b)
      view.setUint8(offset++, color.a)
    }
  }

  // Write frame data
  for (const frame of frames) {
    const u8 = new Uint8Array(buffer, offset, frame.length)
    u8.set(frame)
    offset += frame.length
  }

  return new Blob([buffer], { type: 'application/octet-stream' })
}
```

### Usage Example

```typescript
// Create a simple 16×16 sprite with 4 frames
const frames = [
  new Uint8Array(256),  // Frame 0 (16×16 = 256 pixels)
  new Uint8Array(256),  // Frame 1
  new Uint8Array(256),  // Frame 2
  new Uint8Array(256)   // Frame 3
]

// Create palette (256 colors)
const palette = Array.from({ length: 256 }, (_, i) => ({
  r: i,
  g: 255 - i,
  b: 128,
  a: 255
}))

// Generate sprite file
const spriteBlob = createSpriteFile(frames, palette, {
  width: 16,
  height: 16,
  frameCount: 4,
  fps: 10,
  colorFormat: 0  // 8-bit indexed
})

// Upload to Doki OS
await client.uploadSprite(spriteBlob, 0)
```

---

## See Also

- [SPRITE_UPLOAD_API.md](SPRITE_UPLOAD_API.md) - Complete upload API guide
- [ANIMATION_API_REFERENCE.md](../ANIMATION_API_REFERENCE.md) - Animation system reference
- [animation_types.h](../include/doki/animation/animation_types.h) - C++ header definitions

---

**Version History:**
- **1.0** (2025-01-19): Initial specification

**Built with ❤️ for Doki OS**
