# Doki OS - Sprite Upload API

Complete guide for uploading .spr animation files to specific displays via the PWA client.

---

## Overview

Upload converted GIF sprites (.spr files) to Doki OS displays and have them automatically play using the Sprite Player app.

### Key Features

✅ **Per-Display Upload** - Each display (0 or 1) has its own sprite slot
✅ **Auto-Replace** - New uploads automatically replace old sprites (no duplicates)
✅ **Hot-Reload** - If Sprite Player is running, reload app to pick up new sprite
✅ **Memory Efficient** - Only one sprite per display, automatic cleanup

---

## HTTP Endpoint

### Upload Sprite

**Endpoint:**
```
POST http://<ESP32_IP>/api/media/upload
```

**Content-Type:** `multipart/form-data`

**Parameters:**

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `file` | Blob/File | ✅ | The .spr sprite file |
| `display` | number | ✅ | Target display ID (0 or 1) |
| `type` | string | ✅ | Must be `"sprite"` |

**Response:**
```json
{
  "success": true,
  "message": "Upload complete"
}
```

**File Storage:**
- Display 0: `/media/d0_anim.spr`
- Display 1: `/media/d1_anim.spr`

---

## Client Implementation

### TypeScript/JavaScript Example

```typescript
// Add to DokiOSClient class

/**
 * Upload sprite animation to specific display
 * @param spriteBlob - Converted .spr file
 * @param displayId - Target display (0 or 1)
 * @param onProgress - Optional progress callback
 */
async uploadSprite(
  spriteBlob: Blob,
  displayId: number,
  onProgress?: UploadProgressCallback
): Promise<UploadResponse> {
  const formData = new FormData()
  formData.append('file', spriteBlob, 'animation.spr')
  formData.append('display', displayId.toString())
  formData.append('type', 'sprite')

  return await this._uploadWithProgress(
    '/api/media/upload',
    formData,
    onProgress
  )
}
```

### Usage in PWA

```typescript
// Complete workflow: GIF → Sprite → Upload → Play

async function uploadAndPlayAnimation(gifFile: File, displayId: number) {
  try {
    // Step 1: Convert GIF to sprite (client-side)
    const spriteBlob = await SpriteConverter.convertGIF(gifFile, {
      targetWidth: 240,
      maxFrames: 30,
      colors: 256
    })

    console.log(`Converted: ${(spriteBlob.size / 1024).toFixed(1)} KB`)

    // Step 2: Upload sprite to display
    const client = new DokiOSClient('192.168.1.100')

    await client.uploadSprite(
      spriteBlob,
      displayId,
      (progress) => {
        console.log(`Upload: ${progress.percentage}%`)
      }
    )

    console.log('✓ Upload complete!')

    // Step 3: Load Sprite Player app
    await client.loadApp('sprite_player', displayId)

    console.log('✓ Animation playing!')

  } catch (error) {
    console.error('Failed:', error)
  }
}

// Example usage
const gifFile = document.getElementById('fileInput').files[0]
await uploadAndPlayAnimation(gifFile, 0)  // Upload to Display 0
```

### React Hook Example

```typescript
import { useMutation, useQueryClient } from '@tanstack/react-query'

export function useUploadSprite(espIp: string | null) {
  const queryClient = useQueryClient()

  return useMutation({
    mutationFn: async ({
      sprite,
      displayId,
      onProgress
    }: {
      sprite: Blob
      displayId: number
      onProgress?: UploadProgressCallback
    }) => {
      if (!espIp) throw new Error('No ESP32 IP configured')
      const client = new DokiOSClient(espIp)
      return client.uploadSprite(sprite, displayId, onProgress)
    },

    onSuccess: (data, variables) => {
      // Invalidate status to refresh
      queryClient.invalidateQueries({ queryKey: ['status', espIp] })

      toast.success(
        `Sprite uploaded to Display ${variables.displayId}! ` +
        `(${(data.size / 1024).toFixed(1)} KB)`
      )
    },

    onError: (error) => {
      toast.error(`Upload failed: ${error.message}`)
    }
  })
}

// Usage in component
function SpriteUploadButton({ displayId }: { displayId: number }) {
  const uploadSprite = useUploadSprite('192.168.1.100')
  const loadApp = useLoadApp('192.168.1.100')

  const handleUpload = async (gifFile: File) => {
    // Convert GIF to sprite
    const spriteBlob = await SpriteConverter.convertGIF(gifFile)

    // Upload sprite
    await uploadSprite.mutateAsync({
      sprite: spriteBlob,
      displayId,
      onProgress: (p) => console.log(`${p.percentage}%`)
    })

    // Load Sprite Player app
    await loadApp.mutateAsync({
      appId: 'sprite_player',
      displayId
    })
  }

  return (
    <input
      type="file"
      accept=".gif"
      onChange={(e) => handleUpload(e.target.files[0])}
    />
  )
}
```

---

## Complete Workflow

### 1. Convert GIF to Sprite (Client-Side)

See [CLIENT_SIDE_PROCESSING.md](CLIENT_SIDE_PROCESSING.md#gif-to-sprite-conversion) for full implementation.

**Quick Example:**
```typescript
const spriteBlob = await SpriteConverter.convertGIF(gifFile, {
  targetWidth: 240,      // Fit 240×320 display
  targetHeight: null,    // Auto-calculate aspect ratio
  maxFrames: 30,         // Limit to 30 frames
  targetFPS: null,       // Use original FPS
  colors: 256            // 8-bit indexed color
})
```

**Optimization Tips:**
- **Resize to display dimensions** (240×320) before upload
- **Limit frames** to 30-60 for reasonable file size
- **8-bit color** (256 colors) keeps files small
- **Target file size:** 200-500 KB (optimal)

### 2. Upload Sprite

```typescript
await client.uploadSprite(spriteBlob, displayId, (progress) => {
  updateProgressBar(progress.percentage)
})
```

**What Happens:**
1. File uploaded to ESP32 via multipart form-data
2. ESP32 validates .spr magic number (0x444F4B49)
3. Old sprite (if exists) is automatically deleted
4. New sprite saved to `/media/d{displayId}_anim.spr`
5. Response sent back to client

### 3. Load Sprite Player App

```typescript
await client.loadApp('sprite_player', displayId)
```

**What Happens:**
1. ESP32 creates SpritePlayerApp instance
2. App checks if sprite exists: `MediaService::getMediaInfo(displayId, SPRITE)`
3. If exists: Load sprite using AnimationManager
4. If not: Show placeholder "No sprite uploaded"
5. Animation plays automatically (loops forever)

### 4. Replace Sprite (Hot Update)

To update the sprite while Sprite Player is running:

```typescript
// Upload new sprite
await client.uploadSprite(newSpriteBlob, displayId)

// Reload app to pick up new sprite
await client.loadApp('sprite_player', displayId)
```

**What Happens:**
1. Old sprite deleted automatically (lines 79-82 in media_service.cpp)
2. New sprite saved with same filename
3. App reload picks up new sprite
4. **No ESP32 reset needed!** (~200ms reload)

---

## File Format Validation

### Magic Number Check

The ESP32 validates uploaded .spr files:

```cpp
// Check magic number (0x444F4B49 = "DOKI" in little-endian)
uint32_t magic = *((const uint32_t*)data);
if (magic != 0x444F4B49) {
    return MediaType::UNKNOWN;  // Reject file
}
```

**Client-Side Validation:**
```typescript
async function validateSprite(file: Blob): Promise<boolean> {
  // Read first 4 bytes
  const header = await file.slice(0, 4).arrayBuffer()
  const view = new DataView(header)
  const magic = view.getUint32(0, true)  // Little-endian

  // Check magic number
  const SPRITE_MAGIC = 0x444F4B49
  if (magic !== SPRITE_MAGIC) {
    throw new Error('Invalid sprite file format')
  }

  return true
}

// Usage
await validateSprite(spriteBlob)
await client.uploadSprite(spriteBlob, displayId)
```

---

## Error Handling

### Common Errors

| Error | Cause | Solution |
|-------|-------|----------|
| **400 Bad Request** | Missing parameters | Check `display` and `type` params |
| **400 Invalid sprite file** | Wrong magic number | Ensure .spr file is valid |
| **413 File too large** | File > 5 MB | Reduce frames or dimensions |
| **500 Failed to save** | SPIFFS full | Delete old files |
| **No sprite uploaded** | No file at path | Upload sprite first |

### Error Handling Example

```typescript
async function safeUploadSprite(sprite: Blob, displayId: number) {
  try {
    // Validate file size
    if (sprite.size > 5 * 1024 * 1024) {
      throw new Error('File too large (max 5 MB)')
    }

    // Validate magic number
    await validateSprite(sprite)

    // Upload
    await client.uploadSprite(sprite, displayId)

    // Load app
    await client.loadApp('sprite_player', displayId)

    return { success: true }

  } catch (error) {
    console.error('Upload failed:', error)

    if (error.message.includes('400')) {
      return { success: false, error: 'Invalid sprite file' }
    } else if (error.message.includes('413')) {
      return { success: false, error: 'File too large' }
    } else if (error.message.includes('500')) {
      return { success: false, error: 'ESP32 storage full' }
    } else {
      return { success: false, error: 'Network error' }
    }
  }
}
```

---

## Testing Guide

### Manual Test

1. **Convert GIF:**
   ```bash
   python3 tools/sprite_converter.py input.gif output.spr --width 240 --height 126
   ```

2. **Upload via curl:**
   ```bash
   curl -X POST http://192.168.1.100/api/media/upload \
     -F "file=@output.spr" \
     -F "display=0" \
     -F "type=sprite"
   ```

3. **Load app via curl:**
   ```bash
   curl -X POST "http://192.168.1.100/api/load?app=sprite_player&display=0"
   ```

4. **Verify:**
   - Display 0 should show animation
   - Check serial logs for confirmation

### PWA Test

```typescript
// Test function
async function testSpriteUpload() {
  console.log('Starting test...')

  // 1. Create test GIF (or load from file)
  const gifFile = new File([/* ... */], 'test.gif', { type: 'image/gif' })

  // 2. Convert to sprite
  console.log('Converting GIF...')
  const spriteBlob = await SpriteConverter.convertGIF(gifFile)
  console.log(`Sprite size: ${spriteBlob.size} bytes`)

  // 3. Upload to Display 0
  console.log('Uploading...')
  await client.uploadSprite(spriteBlob, 0)
  console.log('✓ Upload complete')

  // 4. Load Sprite Player
  console.log('Loading app...')
  await client.loadApp('sprite_player', 0)
  console.log('✓ App loaded')

  // 5. Wait 2 seconds
  await new Promise(resolve => setTimeout(resolve, 2000))

  // 6. Check status
  const status = await client.getStatus()
  console.log('Display 0:', status.display0)

  // Expected: display0.app === 'sprite_player'
  if (status.display0?.app === 'sprite_player') {
    console.log('✅ TEST PASSED')
  } else {
    console.log('❌ TEST FAILED')
  }
}

testSpriteUpload()
```

---

## Integration Checklist

For PWA developers integrating sprite upload:

- [ ] **Client code:**
  - [ ] Add `uploadSprite()` method to DokiOSClient
  - [ ] Implement GIF → Sprite conversion (SpriteConverter.js)
  - [ ] Add progress tracking
  - [ ] Add error handling

- [ ] **UI components:**
  - [ ] File picker (accepts .gif files)
  - [ ] Display selector (0 or 1)
  - [ ] Progress bar
  - [ ] Success/error notifications

- [ ] **Workflow:**
  - [ ] Convert GIF on client side
  - [ ] Upload sprite with progress
  - [ ] Load sprite_player app
  - [ ] Handle errors gracefully

- [ ] **Testing:**
  - [ ] Test with small GIF (< 100 KB)
  - [ ] Test with large GIF (> 1 MB)
  - [ ] Test replace (upload twice)
  - [ ] Test both displays (0 and 1)
  - [ ] Test error cases

---

## Performance Considerations

### File Size Guidelines

| Animation | Dimensions | Frames | FPS | Expected Size | Status |
|-----------|------------|--------|-----|---------------|--------|
| Icon | 64×64 | 10 | 15 | ~50 KB | ✅ Optimal |
| Small | 100×100 | 20 | 20 | ~200 KB | ✅ Good |
| Medium | 240×126 | 30 | 10 | ~300 KB | ✅ Good |
| Large | 240×240 | 60 | 15 | ~800 KB | ⚠️ OK |
| Too Large | 240×320 | 120 | 30 | ~2 MB | ❌ Avoid |

### Optimization Tips

1. **Resize before conversion:**
   ```typescript
   // Target display resolution
   const targetWidth = 240
   const targetHeight = 126  // Maintains 1.9 aspect ratio

   const sprite = await SpriteConverter.convertGIF(gifFile, {
     targetWidth,
     targetHeight,
     maxFrames: 30,
     colors: 256
   })
   ```

2. **Limit frames:**
   - 10-20 frames: Short loops, icons
   - 20-40 frames: Medium animations
   - 40-60 frames: Complex animations
   - > 60 frames: Usually unnecessary

3. **Reduce colors:**
   - 256 colors (8-bit): Default, good for most cases
   - 128 colors: Slightly smaller, may show banding
   - 64 colors: Much smaller, noticeable quality loss

4. **Adjust FPS:**
   - 10 FPS: Smooth enough for most animations
   - 15 FPS: Good balance
   - 20-30 FPS: Buttery smooth (larger files)

---

## Advanced Features

### Multi-Display Management

```typescript
// Upload different sprites to each display
async function setupDualDisplay() {
  const client = new DokiOSClient('192.168.1.100')

  // Display 0: Weather animation
  const weather = await SpriteConverter.convertGIF(weatherGif)
  await client.uploadSprite(weather, 0)
  await client.loadApp('sprite_player', 0)

  // Display 1: Clock animation
  const clock = await SpriteConverter.convertGIF(clockGif)
  await client.uploadSprite(clock, 1)
  await client.loadApp('sprite_player', 1)
}
```

### Auto-Update Loop

```typescript
// Periodically update animation (e.g., weather updates)
async function startAutoUpdate(displayId: number, intervalMs: number) {
  setInterval(async () => {
    try {
      // Fetch latest data
      const weatherData = await fetchWeather()

      // Generate animation
      const gifBlob = await generateWeatherAnimation(weatherData)

      // Convert to sprite
      const sprite = await SpriteConverter.convertGIF(gifBlob)

      // Upload and reload
      await client.uploadSprite(sprite, displayId)
      await client.loadApp('sprite_player', displayId)

      console.log('✓ Animation updated')
    } catch (error) {
      console.error('Update failed:', error)
    }
  }, intervalMs)
}

// Update every 10 minutes
startAutoUpdate(0, 10 * 60 * 1000)
```

### Playlist Support (Future Enhancement)

```typescript
// Rotate through multiple sprites (requires firmware enhancement)
async function playPlaylist(displayId: number, sprites: Blob[]) {
  for (const sprite of sprites) {
    await client.uploadSprite(sprite, displayId)
    await client.loadApp('sprite_player', displayId)
    await new Promise(resolve => setTimeout(resolve, 5000))  // 5 sec each
  }
}
```

---

## Troubleshooting

### Issue: "No sprite uploaded" on display

**Causes:**
1. Upload failed (check network)
2. Wrong display ID
3. File not saved to SPIFFS

**Debug:**
```bash
# Check ESP32 serial logs
[MediaService] Saving media to Display 0, type: 4
[MediaService] ✓ Media saved successfully: /media/d0_anim.spr
```

**Solution:**
```typescript
// Verify upload before loading app
await client.uploadSprite(sprite, displayId)

// Wait for ESP32 to finish saving
await new Promise(resolve => setTimeout(resolve, 1000))

// Now load app
await client.loadApp('sprite_player', displayId)
```

### Issue: Animation not playing

**Causes:**
1. Corrupt .spr file
2. Invalid magic number
3. Memory allocation failed

**Debug:**
```bash
# Check logs
[SpriteSheet] Error: Invalid magic number (got 0x..., expected 0x444F4B49)
[SpriteSheet] Error: PSRAM allocation failed
```

**Solution:**
- Validate sprite file before upload
- Reduce sprite size if memory issues

### Issue: Old animation still showing

**Causes:**
1. App not reloaded after upload
2. Browser cache (if fetching via HTTP)

**Solution:**
```typescript
// Force reload
await client.loadApp('sprite_player', displayId)

// Or: Unload then load
await client.unloadApp(displayId)
await client.loadApp('sprite_player', displayId)
```

---

## References

- [PWA_DEVELOPER_GUIDE.md](PWA_DEVELOPER_GUIDE.md) - Complete PWA implementation guide
- [CLIENT_SIDE_PROCESSING.md](CLIENT_SIDE_PROCESSING.md) - GIF conversion details
- [HTTP_REST_API.md](HTTP_REST_API.md) - Complete HTTP API reference
- [JAVASCRIPT_API.md](JAVASCRIPT_API.md) - JavaScript app development

---

**Built with ❤️ for Doki OS**

*Seamless GIF → Sprite upload with per-display management and auto-replace!*
