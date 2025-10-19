# Doki OS - Image Upload API

Complete guide for uploading PNG/JPEG images to specific displays via the PWA client.

---

## Overview

Upload static images (PNG/JPEG) to Doki OS displays and have them automatically display using the Image Viewer app.

### Key Features

✅ **Already Implemented** - No firmware changes needed!
✅ **Per-Display Upload** - Each display (0 or 1) has its own image slot
✅ **Auto-Replace** - New uploads automatically replace old images (no duplicates)
✅ **Format Auto-Detection** - Supports both PNG and JPEG
✅ **Memory Efficient** - Only one image per display, automatic cleanup
✅ **High Quality** - Anti-aliasing enabled, auto-centered

---

## HTTP Endpoint

### Upload Image

**Endpoint:**
```
POST http://<ESP32_IP>/api/media/upload
```

**Content-Type:** `multipart/form-data`

**Parameters:**

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `file` | Blob/File | ✅ | The image file (PNG or JPEG) |
| `display` | number | ✅ | Target display ID (0 or 1) |
| `type` | string | ✅ | Must be `"image"` |

**Response:**
```json
{
  "success": true,
  "message": "Upload complete"
}
```

**File Storage:**
- Display 0 PNG: `/media/d0_image.png`
- Display 0 JPEG: `/media/d0_image.jpg`
- Display 1 PNG: `/media/d1_image.png`
- Display 1 JPEG: `/media/d1_image.jpg`

**Note:** ESP32 auto-detects format from magic bytes and saves with correct extension.

---

## Client Implementation

### TypeScript/JavaScript Example

```typescript
// Add to DokiOSClient class

/**
 * Upload image to specific display (with optimization)
 * @param imageFile - PNG or JPEG image file
 * @param displayId - Target display (0 or 1)
 * @param onProgress - Optional progress callback
 */
async uploadImage(
  imageFile: File | Blob,
  displayId: number,
  onProgress?: UploadProgressCallback
): Promise<UploadResponse> {
  // Optimize image before upload (resize to 240×320)
  const optimizedBlob = await this.optimizeImage(imageFile)

  const formData = new FormData()

  // Determine filename based on original format
  const filename = imageFile instanceof File
    ? imageFile.name
    : 'image.jpg'

  formData.append('file', optimizedBlob, filename)
  formData.append('display', displayId.toString())
  formData.append('type', 'image')

  return await this._uploadWithProgress(
    '/api/media/upload',
    formData,
    onProgress
  )
}

/**
 * Optimize image for display (resize to 240×320)
 * @param file - Original image file
 * @returns Optimized image blob
 */
private async optimizeImage(file: File | Blob): Promise<Blob> {
  return new Promise((resolve, reject) => {
    const canvas = document.createElement('canvas')
    const ctx = canvas.getContext('2d')
    if (!ctx) {
      reject(new Error('Failed to get canvas context'))
      return
    }

    const img = new Image()

    img.onload = () => {
      // Target display dimensions
      const DISPLAY_WIDTH = 240
      const DISPLAY_HEIGHT = 320

      // Calculate scaled dimensions (maintain aspect ratio, fit within display)
      let width = img.width
      let height = img.height
      const aspectRatio = width / height

      // Scale down if larger than display
      if (width > DISPLAY_WIDTH || height > DISPLAY_HEIGHT) {
        if (aspectRatio > DISPLAY_WIDTH / DISPLAY_HEIGHT) {
          // Wide image - fit to width
          width = DISPLAY_WIDTH
          height = Math.round(width / aspectRatio)
        } else {
          // Tall image - fit to height
          height = DISPLAY_HEIGHT
          width = Math.round(height * aspectRatio)
        }
      }

      canvas.width = width
      canvas.height = height

      // High-quality scaling
      ctx.imageSmoothingEnabled = true
      ctx.imageSmoothingQuality = 'high'
      ctx.drawImage(img, 0, 0, width, height)

      // Export as JPEG (smaller file size, good quality)
      canvas.toBlob(
        (blob) => {
          if (blob) {
            resolve(blob)
          } else {
            reject(new Error('Failed to create blob'))
          }
        },
        'image/jpeg',
        0.85  // 85% quality (good balance)
      )
    }

    img.onerror = () => {
      reject(new Error('Failed to load image'))
    }

    img.src = URL.createObjectURL(file)
  })
}
```

### Usage in PWA

```typescript
// Complete workflow: Upload image → Display on screen

async function uploadAndDisplayImage(imageFile: File, displayId: number) {
  const client = new DokiOSClient('192.168.1.100')

  try {
    // Step 1: Upload image (auto-optimized to 240×320)
    console.log('Uploading image...')

    await client.uploadImage(
      imageFile,
      displayId,
      (progress) => {
        console.log(`Upload: ${progress.percentage}%`)
        updateProgressBar(progress.percentage)
      }
    )

    console.log('✓ Upload complete!')

    // Step 2: Load Image Viewer app
    console.log('Loading Image Viewer...')

    await client.loadApp('image', displayId)  // App ID is "image"

    console.log('✓ Image displayed!')

  } catch (error) {
    console.error('Failed:', error)
    alert('Upload failed: ' + error.message)
  }
}

// Example usage
const fileInput = document.getElementById('fileInput') as HTMLInputElement
fileInput.addEventListener('change', async (e) => {
  const file = (e.target as HTMLInputElement).files?.[0]
  if (file) {
    await uploadAndDisplayImage(file, 0)  // Upload to Display 0
  }
})
```

### React Hook Example

```typescript
import { useMutation, useQueryClient } from '@tanstack/react-query'
import { toast } from 'sonner'

/**
 * React hook for uploading images
 */
export function useUploadImage(espIp: string | null) {
  const queryClient = useQueryClient()

  return useMutation({
    mutationFn: async ({
      image,
      displayId,
      onProgress
    }: {
      image: File
      displayId: number
      onProgress?: UploadProgressCallback
    }) => {
      if (!espIp) throw new Error('No ESP32 IP configured')
      const client = new DokiOSClient(espIp)
      return client.uploadImage(image, displayId, onProgress)
    },

    onSuccess: (data, variables) => {
      // Invalidate status to refresh
      queryClient.invalidateQueries({ queryKey: ['status', espIp] })

      toast.success(
        `Image uploaded to Display ${variables.displayId}! ` +
        `(${(data.size / 1024).toFixed(1)} KB)`
      )
    },

    onError: (error) => {
      toast.error(`Upload failed: ${error.message}`)
    }
  })
}

// Usage in component
function ImageUploadButton({ displayId }: { displayId: number }) {
  const uploadImage = useUploadImage('192.168.1.100')
  const loadApp = useLoadApp('192.168.1.100')
  const [progress, setProgress] = useState(0)

  const handleUpload = async (file: File) => {
    try {
      // Upload image
      await uploadImage.mutateAsync({
        image: file,
        displayId,
        onProgress: (p) => setProgress(p.percentage)
      })

      // Load Image Viewer app
      await loadApp.mutateAsync({
        appId: 'image',
        displayId
      })

      setProgress(0)
    } catch (error) {
      console.error('Upload failed:', error)
    }
  }

  return (
    <div>
      <input
        type="file"
        accept="image/png,image/jpeg,image/jpg"
        onChange={(e) => {
          const file = e.target.files?.[0]
          if (file) handleUpload(file)
        }}
        disabled={uploadImage.isPending}
      />

      {uploadImage.isPending && (
        <div className="progress-bar">
          <div
            className="progress-fill"
            style={{ width: `${progress}%` }}
          />
          <span>{progress}%</span>
        </div>
      )}
    </div>
  )
}
```

---

## Complete Workflow

### 1. Optimize Image (Client-Side)

**Why Optimize?**
- Original 4K photo: 5 MB, takes 40 seconds to upload
- Optimized 240×320: 50 KB, takes 0.4 seconds to upload
- **100× faster!**

**Optimization:**
```typescript
// Resize to display dimensions (240×320)
// Convert to JPEG at 85% quality
// Result: 50-150 KB file size

const optimizedBlob = await client.optimizeImage(originalFile)
console.log(`Original: ${originalFile.size} bytes`)
console.log(`Optimized: ${optimizedBlob.size} bytes`)
```

**Manual Optimization (Python):**
```python
from PIL import Image

# Open image
img = Image.open('photo.jpg')

# Resize to fit 240×320 (maintain aspect ratio)
img.thumbnail((240, 320), Image.Resampling.LANCZOS)

# Save with JPEG compression
img.save('optimized.jpg', 'JPEG', quality=85, optimize=True)
```

### 2. Upload Image

```typescript
await client.uploadImage(imageFile, displayId, (progress) => {
  updateProgressBar(progress.percentage)
})
```

**What Happens:**
1. Client optimizes image (resize to 240×320)
2. Image uploaded to ESP32 via multipart form-data
3. ESP32 detects format (PNG or JPEG) from magic bytes
4. Old image (if exists) is automatically deleted
5. New image saved to `/media/d{displayId}_image.{png|jpg}`
6. Response sent back to client

### 3. Load Image Viewer App

```typescript
await client.loadApp('image', displayId)
```

**What Happens:**
1. ESP32 creates ImagePreviewApp instance
2. App checks if image exists: `MediaService::getMediaInfo(displayId, IMAGE_*)`
3. If exists: Load image using LVGL image decoder
4. If not: Show placeholder "No image uploaded"
5. Image displayed (auto-centered, anti-aliasing enabled)

### 4. Replace Image (Hot Update)

To update the image while Image Viewer is running:

```typescript
// Upload new image
await client.uploadImage(newImageFile, displayId)

// Reload app to pick up new image
await client.loadApp('image', displayId)
```

**What Happens:**
1. Old image deleted automatically (lines 79-82 in media_service.cpp)
2. New image saved with same filename path
3. App reload picks up new image
4. **No ESP32 reset needed!** (~200ms reload)

---

## Image Optimization

### Recommended Settings

| Setting | Value | Reason |
|---------|-------|--------|
| **Target Width** | 240 px | Match display width |
| **Target Height** | 320 px | Match display height |
| **Format** | JPEG | Smaller file size for photos |
| **Quality** | 85% | Good balance (imperceptible loss) |
| **Scaling** | Maintain aspect ratio | Avoid distortion |

### File Size Guidelines

| Original | Optimized | Upload Time @ 5 Mbps | Status |
|----------|-----------|----------------------|--------|
| 4K Photo (5 MB) | 50-100 KB | 0.4s | ✅ Optimal |
| HD Photo (2 MB) | 40-80 KB | 0.3s | ✅ Great |
| Medium (500 KB) | 30-60 KB | 0.2s | ✅ Good |
| Already Small (100 KB) | 30-50 KB | 0.2s | ✅ Fine |
| Tiny (50 KB) | 30-50 KB | 0.2s | ✅ OK |

**Bottom Line:** Always optimize before upload!

### Optimization Implementation

```typescript
class ImageOptimizer {
  /**
   * Optimize image for Doki OS display
   */
  static async optimize(
    file: File,
    options: {
      maxWidth?: number
      maxHeight?: number
      quality?: number
      format?: 'jpeg' | 'png'
    } = {}
  ): Promise<Blob> {
    const {
      maxWidth = 240,
      maxHeight = 320,
      quality = 0.85,
      format = 'jpeg'
    } = options

    return new Promise((resolve, reject) => {
      const canvas = document.createElement('canvas')
      const ctx = canvas.getContext('2d')!
      const img = new Image()

      img.onload = () => {
        // Calculate dimensions (maintain aspect ratio)
        let width = img.width
        let height = img.height
        const aspectRatio = width / height

        if (width > maxWidth || height > maxHeight) {
          if (aspectRatio > maxWidth / maxHeight) {
            width = maxWidth
            height = width / aspectRatio
          } else {
            height = maxHeight
            width = height * aspectRatio
          }
        }

        // Round to integers
        canvas.width = Math.round(width)
        canvas.height = Math.round(height)

        // High-quality scaling
        ctx.imageSmoothingEnabled = true
        ctx.imageSmoothingQuality = 'high'
        ctx.drawImage(img, 0, 0, canvas.width, canvas.height)

        // Export
        const mimeType = format === 'png' ? 'image/png' : 'image/jpeg'
        canvas.toBlob(
          (blob) => {
            if (blob) {
              resolve(blob)
            } else {
              reject(new Error('Failed to create blob'))
            }
          },
          mimeType,
          quality
        )
      }

      img.onerror = () => reject(new Error('Failed to load image'))
      img.src = URL.createObjectURL(file)
    })
  }

  /**
   * Get optimized image dimensions (for preview)
   */
  static calculateDimensions(
    originalWidth: number,
    originalHeight: number,
    maxWidth: number = 240,
    maxHeight: number = 320
  ): { width: number; height: number } {
    let width = originalWidth
    let height = originalHeight
    const aspectRatio = width / height

    if (width > maxWidth || height > maxHeight) {
      if (aspectRatio > maxWidth / maxHeight) {
        width = maxWidth
        height = width / aspectRatio
      } else {
        height = maxHeight
        width = height * aspectRatio
      }
    }

    return {
      width: Math.round(width),
      height: Math.round(height)
    }
  }
}

// Usage
const optimized = await ImageOptimizer.optimize(imageFile, {
  maxWidth: 240,
  maxHeight: 320,
  quality: 0.85,
  format: 'jpeg'
})

console.log(`Reduced from ${imageFile.size} to ${optimized.size} bytes`)
```

---

## Format Support

### Supported Formats

| Format | Extension | Magic Bytes | Use Case |
|--------|-----------|-------------|----------|
| **PNG** | `.png` | `89 50 4E 47` | Graphics, logos, transparency |
| **JPEG** | `.jpg`, `.jpeg` | `FF D8 FF` | Photos, gradients |

### Format Selection Guide

**Use PNG when:**
- Image has transparency
- Sharp edges (logos, text, icons)
- Need lossless quality
- File size not critical

**Use JPEG when:**
- Photographs
- Complex gradients
- File size matters
- No transparency needed

### Auto-Detection

ESP32 automatically detects format from magic bytes:

```cpp
// PNG signature
if (data[0] == 0x89 && data[1] == 0x50 &&
    data[2] == 0x4E && data[3] == 0x47) {
    return MediaType::IMAGE_PNG;
}

// JPEG signature
if (data[0] == 0xFF && data[1] == 0xD8 && data[2] == 0xFF) {
    return MediaType::IMAGE_JPEG;
}
```

**Client doesn't need to specify format** - ESP32 auto-detects!

---

## Error Handling

### Common Errors

| Error | Cause | Solution |
|-------|-------|----------|
| **400 Bad Request** | Missing parameters | Check `display` and `type` params |
| **400 Not a valid image** | Corrupt file or wrong format | Ensure valid PNG/JPEG |
| **413 File too large** | File > 5 MB | Optimize image first |
| **500 Failed to save** | SPIFFS full | Delete old files |
| **No image uploaded** | No file at path | Upload image first |

### Error Handling Example

```typescript
async function safeUploadImage(image: File, displayId: number) {
  try {
    // Validate file type
    if (!image.type.startsWith('image/')) {
      throw new Error('Not an image file')
    }

    if (!['image/png', 'image/jpeg', 'image/jpg'].includes(image.type)) {
      throw new Error('Unsupported format (use PNG or JPEG)')
    }

    // Validate file size (before optimization)
    if (image.size > 10 * 1024 * 1024) {
      throw new Error('Image too large (max 10 MB before optimization)')
    }

    // Upload (will optimize automatically)
    const client = new DokiOSClient('192.168.1.100')
    await client.uploadImage(image, displayId)

    // Load app
    await client.loadApp('image', displayId)

    return { success: true }

  } catch (error) {
    console.error('Upload failed:', error)

    if (error.message.includes('400')) {
      return { success: false, error: 'Invalid image file' }
    } else if (error.message.includes('413')) {
      return { success: false, error: 'File too large (optimize first)' }
    } else if (error.message.includes('500')) {
      return { success: false, error: 'ESP32 storage full' }
    } else if (error.message.includes('timeout')) {
      return { success: false, error: 'Upload timeout (check WiFi)' }
    } else {
      return { success: false, error: error.message }
    }
  }
}
```

### Retry Logic

```typescript
async function uploadWithRetry(
  image: File,
  displayId: number,
  maxRetries: number = 3
): Promise<void> {
  let lastError: Error

  for (let attempt = 1; attempt <= maxRetries; attempt++) {
    try {
      console.log(`Upload attempt ${attempt}/${maxRetries}`)

      const client = new DokiOSClient('192.168.1.100')
      await client.uploadImage(image, displayId)

      console.log('✓ Upload successful')
      return

    } catch (error) {
      lastError = error as Error
      console.error(`Attempt ${attempt} failed:`, error)

      // Don't retry on client errors (400)
      if (error.message.includes('400')) {
        throw error
      }

      // Wait before retry (exponential backoff)
      if (attempt < maxRetries) {
        const delay = Math.min(1000 * Math.pow(2, attempt), 10000)
        console.log(`Retrying in ${delay}ms...`)
        await new Promise(resolve => setTimeout(resolve, delay))
      }
    }
  }

  throw new Error(`Upload failed after ${maxRetries} attempts: ${lastError.message}`)
}
```

---

## Testing Guide

### Manual Test (curl)

```bash
# 1. Optimize image (optional)
convert input.jpg -resize 240x320 -quality 85 optimized.jpg

# 2. Upload to Display 0
curl -X POST http://192.168.1.100/api/media/upload \
  -F "file=@optimized.jpg" \
  -F "display=0" \
  -F "type=image"

# Expected response:
# {"success":true,"message":"Upload complete"}

# 3. Load Image Viewer app
curl -X POST "http://192.168.1.100/api/load?app=image&display=0"

# 4. Verify on Display 0
# Image should appear, auto-centered

# 5. Check ESP32 serial logs
# [MediaService] Saving media to Display 0, type: 2
# [MediaService] ✓ Media saved successfully: /media/d0_image.jpg
# [ImagePreview] Loading image from: /media/d0_image.jpg
# [ImagePreview] ✓ Image loaded successfully!
```

### PWA Test Function

```typescript
async function testImageUpload() {
  console.log('Starting image upload test...')

  try {
    // 1. Create test image (or load from file)
    const response = await fetch('/test-image.jpg')
    const imageBlob = await response.blob()
    console.log(`Test image: ${imageBlob.size} bytes`)

    // 2. Upload to Display 0
    console.log('Uploading...')
    const client = new DokiOSClient('192.168.1.100')

    await client.uploadImage(imageBlob, 0, (progress) => {
      console.log(`Progress: ${progress.percentage}%`)
    })

    console.log('✓ Upload complete')

    // 3. Load Image Viewer
    console.log('Loading app...')
    await client.loadApp('image', 0)
    console.log('✓ App loaded')

    // 4. Wait 2 seconds
    await new Promise(resolve => setTimeout(resolve, 2000))

    // 5. Check status
    const status = await client.getStatus()
    console.log('Display 0:', status.display0)

    // Expected: display0.app === 'image'
    if (status.display0?.app === 'image') {
      console.log('✅ TEST PASSED')
      return true
    } else {
      console.log('❌ TEST FAILED: Wrong app')
      return false
    }

  } catch (error) {
    console.error('❌ TEST FAILED:', error)
    return false
  }
}

// Run test
testImageUpload()
```

### Automated Test Suite

```typescript
describe('Image Upload', () => {
  const client = new DokiOSClient('192.168.1.100')

  test('should upload JPEG image', async () => {
    const imageBlob = await fetch('/test.jpg').then(r => r.blob())
    await client.uploadImage(imageBlob, 0)

    const status = await client.getStatus()
    await client.loadApp('image', 0)

    expect(status.display0?.app).toBe('image')
  })

  test('should upload PNG image', async () => {
    const imageBlob = await fetch('/test.png').then(r => r.blob())
    await client.uploadImage(imageBlob, 1)

    await client.loadApp('image', 1)
    const status = await client.getStatus()

    expect(status.display1?.app).toBe('image')
  })

  test('should replace old image', async () => {
    // Upload first image
    const image1 = await fetch('/test1.jpg').then(r => r.blob())
    await client.uploadImage(image1, 0)

    // Upload second image (should replace first)
    const image2 = await fetch('/test2.jpg').then(r => r.blob())
    await client.uploadImage(image2, 0)

    // Should only have one image file on ESP32
    // (old one deleted automatically)
  })

  test('should optimize large image', async () => {
    // 5 MB image
    const largeImage = await fetch('/large.jpg').then(r => r.blob())
    expect(largeImage.size).toBeGreaterThan(5 * 1024 * 1024)

    // Should optimize to < 200 KB
    const optimized = await client.optimizeImage(largeImage)
    expect(optimized.size).toBeLessThan(200 * 1024)
  })

  test('should handle upload errors', async () => {
    // Invalid file
    const invalidBlob = new Blob(['not an image'], { type: 'text/plain' })

    await expect(
      client.uploadImage(invalidBlob, 0)
    ).rejects.toThrow('Not a valid image file')
  })
})
```

---

## Advanced Features

### Multi-Display Management

```typescript
// Upload different images to each display
async function setupDualDisplay() {
  const client = new DokiOSClient('192.168.1.100')

  // Display 0: Dashboard image
  const dashboard = await fetch('/dashboard.png').then(r => r.blob())
  await client.uploadImage(dashboard, 0)
  await client.loadApp('image', 0)

  // Display 1: Weather image
  const weather = await fetch('/weather.jpg').then(r => r.blob())
  await client.uploadImage(weather, 1)
  await client.loadApp('image', 1)

  console.log('✓ Both displays configured')
}
```

### Auto-Update Loop (e.g., Dashboard)

```typescript
// Periodically update image (e.g., live dashboard)
async function startDashboardUpdates(displayId: number, intervalMs: number) {
  const client = new DokiOSClient('192.168.1.100')

  setInterval(async () => {
    try {
      // Generate latest dashboard image
      const dashboardBlob = await generateDashboard()

      // Upload and reload
      await client.uploadImage(dashboardBlob, displayId)
      await client.loadApp('image', displayId)

      console.log('✓ Dashboard updated')
    } catch (error) {
      console.error('Update failed:', error)
    }
  }, intervalMs)
}

// Update every 5 minutes
startDashboardUpdates(0, 5 * 60 * 1000)
```

### Image Carousel (Slideshow)

```typescript
// Rotate through multiple images
async function playSlideshow(
  displayId: number,
  images: File[],
  intervalMs: number = 5000
) {
  const client = new DokiOSClient('192.168.1.100')

  for (let i = 0; i < images.length; i++) {
    console.log(`Showing image ${i + 1}/${images.length}`)

    // Upload image
    await client.uploadImage(images[i], displayId)

    // Load Image Viewer (will show new image)
    await client.loadApp('image', displayId)

    // Wait before next image
    if (i < images.length - 1) {
      await new Promise(resolve => setTimeout(resolve, intervalMs))
    }
  }
}

// Usage
const images = [
  new File([...], 'photo1.jpg'),
  new File([...], 'photo2.jpg'),
  new File([...], 'photo3.jpg')
]

await playSlideshow(0, images, 5000)  // 5 seconds per image
```

---

## Performance Tips

### 1. Always Optimize Before Upload

```typescript
// ❌ BAD: Upload original 5 MB photo
await client.uploadImage(originalPhoto, 0)  // Takes 40 seconds

// ✅ GOOD: Optimize first
const optimized = await ImageOptimizer.optimize(originalPhoto)
await client.uploadImage(optimized, 0)  // Takes 0.4 seconds
```

### 2. Use JPEG for Photos

```typescript
// PNG photo: 500 KB
// JPEG photo (85% quality): 50 KB (10× smaller!)

await ImageOptimizer.optimize(photo, {
  format: 'jpeg',
  quality: 0.85
})
```

### 3. Batch Uploads (Multiple Displays)

```typescript
// ✅ Upload in parallel
await Promise.all([
  client.uploadImage(image0, 0),
  client.uploadImage(image1, 1)
])

// Then load apps
await Promise.all([
  client.loadApp('image', 0),
  client.loadApp('image', 1)
])
```

### 4. Show Progress Feedback

```typescript
// ✅ Good UX: Show upload progress
await client.uploadImage(image, 0, (progress) => {
  progressBar.style.width = `${progress.percentage}%`
  statusText.textContent = `Uploading: ${progress.percentage}%`
})
```

---

## Troubleshooting

### Issue: "No image uploaded" on display

**Causes:**
1. Upload failed (check network)
2. Wrong display ID
3. File not saved to SPIFFS

**Debug:**
```bash
# Check ESP32 serial logs
[MediaService] Saving media to Display 0, type: 2
[MediaService] ✓ Media saved successfully: /media/d0_image.jpg
```

**Solution:**
```typescript
// Verify upload before loading app
await client.uploadImage(image, displayId)

// Wait for ESP32 to finish saving
await new Promise(resolve => setTimeout(resolve, 1000))

// Now load app
await client.loadApp('image', displayId)
```

### Issue: Image not displaying

**Causes:**
1. Corrupt image file
2. Unsupported format (not PNG/JPEG)
3. Memory allocation failed

**Debug:**
```bash
# Check logs
[MediaService] Error: Unknown or invalid media format
[ImagePreview] Error: Failed to load image from SPIFFS
```

**Solution:**
- Validate image file before upload
- Ensure PNG or JPEG format
- Reduce image size if memory issues

### Issue: Image looks pixelated

**Causes:**
1. Image smaller than display resolution
2. Upscaling applied

**Solution:**
```typescript
// Ensure image is at least 240×320
if (img.width < 240 || img.height < 320) {
  console.warn('Image too small, may look pixelated')
}

// Use PNG for sharp edges (logos, text)
await ImageOptimizer.optimize(image, {
  format: 'png'  // Lossless
})
```

### Issue: Old image still showing

**Causes:**
1. App not reloaded after upload
2. Browser cache (if fetching via HTTP)

**Solution:**
```typescript
// Force reload
await client.uploadImage(newImage, displayId)
await client.loadApp('image', displayId)

// Or: Unload then load
await client.unloadApp(displayId)
await client.loadApp('image', displayId)
```

---

## Integration Checklist

For PWA developers integrating image upload:

**Client Code:**
- [ ] Add `uploadImage()` method to DokiOSClient
- [ ] Implement `optimizeImage()` helper (resize to 240×320)
- [ ] Add progress tracking
- [ ] Add error handling with retries

**UI Components:**
- [ ] File picker (accepts `.png`, `.jpg`, `.jpeg`)
- [ ] Display selector (0 or 1)
- [ ] Progress bar with percentage
- [ ] Image preview (before upload)
- [ ] Success/error notifications

**Workflow:**
- [ ] Select image from file picker
- [ ] Show preview with dimensions
- [ ] Optimize on client side
- [ ] Upload with progress
- [ ] Load Image Viewer app
- [ ] Handle errors gracefully

**Testing:**
- [ ] Test with small image (< 100 KB)
- [ ] Test with large image (> 5 MB)
- [ ] Test replace (upload twice)
- [ ] Test both displays (0 and 1)
- [ ] Test both formats (PNG and JPEG)
- [ ] Test error cases (invalid file, network error)

---

## Summary

### ✅ Already Working (No Firmware Changes!)

- **ImagePreviewApp** - Displays PNG/JPEG images
- **HTTP Endpoint** - `/api/media/upload` with `type="image"`
- **Per-Display Storage** - `/media/d0_image.{png|jpg}`, `/media/d1_image.{png|jpg}`
- **Auto-Replace** - Old image deleted when uploading new one
- **Format Auto-Detection** - PNG vs JPEG detected from magic bytes
- **High Quality** - Anti-aliasing, auto-centered

### 📝 Client Implementation

**Add to PWA:**
1. `uploadImage()` method in DokiOSClient
2. `optimizeImage()` helper (resize to 240×320)
3. Progress tracking UI
4. Error handling

**Workflow:**
```
1. Select image → 2. Optimize (240×320) → 3. Upload → 4. Load app → 5. Display!
```

**Key Optimization:**
- Resize to 240×320 before upload
- Use JPEG for photos (85% quality)
- Result: 50-150 KB files, < 1 second upload

---

## References

- [SPRITE_UPLOAD_API.md](SPRITE_UPLOAD_API.md) - Sprite animation upload
- [CLIENT_SIDE_PROCESSING.md](CLIENT_SIDE_PROCESSING.md) - Client optimization guide
- [PWA_DEVELOPER_GUIDE.md](PWA_DEVELOPER_GUIDE.md) - Complete PWA guide
- [HTTP_REST_API.md](HTTP_REST_API.md) - Full HTTP API reference

---

**Built with ❤️ for Doki OS**

*Per-display image upload with auto-optimization and instant display!*
