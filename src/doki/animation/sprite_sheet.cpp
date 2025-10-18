/**
 * @file sprite_sheet.cpp
 * @brief Implementation of SpriteSheet class
 */

#include "doki/animation/sprite_sheet.h"
#include <esp_heap_caps.h>

namespace Doki {
namespace Animation {

// ==========================================
// Constructor / Destructor
// ==========================================

SpriteSheet::SpriteSheet()
    : _loaded(false),
      _lastError(AnimationError::NONE),
      _loadTimeMs(0),
      _memoryUsed(0),
      _palette(nullptr),
      _paletteRGB565(nullptr),
      _frameData(nullptr),
      _frameMetadata(nullptr),
      _frameDataSize(0),
      _singleFrameSize(0) {
    memset(&_header, 0, sizeof(SpriteHeader));
}

SpriteSheet::~SpriteSheet() {
    unload();
}

// ==========================================
// Loading Methods
// ==========================================

bool SpriteSheet::loadFromFile(const char* filepath) {
    Serial.printf("[SpriteSheet] Loading from file: %s\n", filepath);
    uint32_t startTime = millis();

    // Check if already loaded
    if (_loaded) {
        Serial.println("[SpriteSheet] Error: Already loaded");
        _lastError = AnimationError::ALREADY_LOADED;
        return false;
    }

    // Read file from filesystem
    uint8_t* data = nullptr;
    size_t size = 0;

    if (!FilesystemManager::readFile(filepath, &data, size)) {
        Serial.printf("[SpriteSheet] Error: Failed to read file: %s\n", filepath);
        _lastError = AnimationError::FILE_NOT_FOUND;
        return false;
    }

    // Parse sprite data
    bool success = parseSprite(data, size);

    // Free temporary buffer
    delete[] data;

    if (success) {
        _loadTimeMs = millis() - startTime;
        Serial.printf("[SpriteSheet] ✓ Loaded successfully in %ums\n", _loadTimeMs);
        Serial.printf("[SpriteSheet] Memory used: %zu bytes (%zu KB)\n",
                     _memoryUsed, _memoryUsed / 1024);
    }

    return success;
}

bool SpriteSheet::loadFromMemory(const uint8_t* data, size_t size) {
    Serial.printf("[SpriteSheet] Loading from memory (%zu bytes)\n", size);
    uint32_t startTime = millis();

    // Check if already loaded
    if (_loaded) {
        Serial.println("[SpriteSheet] Error: Already loaded");
        _lastError = AnimationError::ALREADY_LOADED;
        return false;
    }

    // Parse sprite data
    bool success = parseSprite(data, size);

    if (success) {
        _loadTimeMs = millis() - startTime;
        Serial.printf("[SpriteSheet] ✓ Loaded successfully in %ums\n", _loadTimeMs);
    }

    return success;
}

void SpriteSheet::unload() {
    if (!_loaded) {
        return;
    }

    Serial.println("[SpriteSheet] Unloading...");

    freeMemory();

    _loaded = false;
    _memoryUsed = 0;
    memset(&_header, 0, sizeof(SpriteHeader));

    Serial.println("[SpriteSheet] ✓ Unloaded");
}

// ==========================================
// Frame Data Access
// ==========================================

const uint8_t* SpriteSheet::getFrameData(uint16_t frameIndex) const {
    if (!_loaded || !isValidFrame(frameIndex) || !_frameData) {
        return nullptr;
    }

    size_t offset = calculateFrameOffset(frameIndex);
    return _frameData + offset;
}

size_t SpriteSheet::getFrameSize(uint16_t frameIndex) const {
    if (!_loaded || !isValidFrame(frameIndex)) {
        return 0;
    }

    // For uncompressed data, all frames are same size
    return _singleFrameSize;
}

const FrameMetadata* SpriteSheet::getFrameMetadata(uint16_t frameIndex) const {
    if (!_loaded || !isValidFrame(frameIndex) || !_frameMetadata) {
        return nullptr;
    }

    return &_frameMetadata[frameIndex];
}

// ==========================================
// Validation & Info
// ==========================================

void SpriteSheet::printInfo() const {
    Serial.println("\n========== Sprite Sheet Info ==========");
    Serial.printf("Loaded: %s\n", _loaded ? "Yes" : "No");

    if (!_loaded) {
        Serial.println("=======================================\n");
        return;
    }

    Serial.printf("Dimensions: %dx%d pixels\n", _header.frameWidth, _header.frameHeight);
    Serial.printf("Frame Count: %d\n", _header.frameCount);
    Serial.printf("FPS: %d\n", _header.fps);
    Serial.printf("Color Format: %d\n", (int)_header.colorFormat);
    Serial.printf("Compression: %d\n", (int)_header.compression);
    Serial.printf("Single Frame Size: %zu bytes\n", _singleFrameSize);
    Serial.printf("Total Frame Data: %zu bytes (%zu KB)\n",
                 _frameDataSize, _frameDataSize / 1024);
    Serial.printf("Memory Used: %zu bytes (%zu KB)\n",
                 _memoryUsed, _memoryUsed / 1024);
    Serial.printf("Load Time: %u ms\n", _loadTimeMs);
    Serial.println("=======================================\n");
}

// ==========================================
// Internal Methods
// ==========================================

bool SpriteSheet::parseSprite(const uint8_t* data, size_t size) {
    // Validate minimum size
    if (size < SPRITE_HEADER_SIZE) {
        Serial.println("[SpriteSheet] Error: Data too small for header");
        _lastError = AnimationError::INVALID_FORMAT;
        return false;
    }

    // Parse header
    memcpy(&_header, data, sizeof(SpriteHeader));

    // Validate header
    if (!validateHeader(_header)) {
        return false;
    }

    // Calculate sizes
    size_t bytesPerPixel = 1;  // 8-bit indexed
    if (_header.colorFormat == ColorFormat::RGB565) bytesPerPixel = 2;
    if (_header.colorFormat == ColorFormat::RGB888) bytesPerPixel = 3;

    _singleFrameSize = _header.frameWidth * _header.frameHeight * bytesPerPixel;
    _frameDataSize = _singleFrameSize * _header.frameCount;

    // Calculate expected file size
    size_t expectedSize = SPRITE_HEADER_SIZE + PALETTE_SIZE + _frameDataSize;
    if (size < expectedSize) {
        Serial.printf("[SpriteSheet] Error: Data too small (got %zu, expected %zu)\n",
                     size, expectedSize);
        _lastError = AnimationError::CORRUPT_DATA;
        return false;
    }

    // Load palette (for indexed color)
    if (_header.colorFormat == ColorFormat::INDEXED_8BIT) {
        if (!loadPalette(data + SPRITE_HEADER_SIZE)) {
            return false;
        }
    }

    // Load frame data
    size_t frameDataOffset = SPRITE_HEADER_SIZE;
    if (_header.colorFormat == ColorFormat::INDEXED_8BIT) {
        frameDataOffset += PALETTE_SIZE;
    }

    if (!loadFrames(data + frameDataOffset, _frameDataSize)) {
        return false;
    }

    _loaded = true;
    _lastError = AnimationError::NONE;

    return true;
}

bool SpriteSheet::validateHeader(const SpriteHeader& header) {
    // Check magic number
    if (header.magic != SPRITE_MAGIC) {
        Serial.printf("[SpriteSheet] Error: Invalid magic number (got 0x%08X, expected 0x%08X)\n",
                     header.magic, SPRITE_MAGIC);
        _lastError = AnimationError::INVALID_FORMAT;
        return false;
    }

    // Check version
    if (header.version != SPRITE_VERSION) {
        Serial.printf("[SpriteSheet] Error: Unsupported version (got %d, expected %d)\n",
                     header.version, SPRITE_VERSION);
        _lastError = AnimationError::UNSUPPORTED_VERSION;
        return false;
    }

    // Validate dimensions
    if (!validateDimensions(header.frameWidth, header.frameHeight)) {
        Serial.printf("[SpriteSheet] Error: Invalid dimensions (%dx%d)\n",
                     header.frameWidth, header.frameHeight);
        _lastError = AnimationError::INVALID_DIMENSIONS;
        return false;
    }

    // Validate frame count
    if (!validateFrameCount(header.frameCount)) {
        Serial.printf("[SpriteSheet] Error: Invalid frame count (%d)\n",
                     header.frameCount);
        _lastError = AnimationError::INVALID_FRAME_COUNT;
        return false;
    }

    // Validate FPS
    if (header.fps == 0 || header.fps > 60) {
        Serial.printf("[SpriteSheet] Warning: Invalid FPS (%d), using 30\n", header.fps);
        _header.fps = 30;  // Default to 30 FPS
    }

    return true;
}

bool SpriteSheet::loadPalette(const uint8_t* data) {
    Serial.println("[SpriteSheet] Loading palette...");

    // Allocate palette in PSRAM
    _palette = (RGBAColor*)allocatePSRAM(PALETTE_SIZE);
    if (!_palette) {
        Serial.println("[SpriteSheet] Error: Failed to allocate palette memory");
        _lastError = AnimationError::OUT_OF_MEMORY;
        return false;
    }

    // Copy palette data
    memcpy(_palette, data, PALETTE_SIZE);
    _memoryUsed += PALETTE_SIZE;

    // Pre-convert palette to RGB565 for fast rendering (CRITICAL OPTIMIZATION)
    Serial.println("[SpriteSheet] Converting palette to RGB565...");
    size_t rgb565Size = 256 * sizeof(uint16_t);  // 256 colors × 2 bytes = 512 bytes
    _paletteRGB565 = (uint16_t*)allocatePSRAM(rgb565Size);
    if (!_paletteRGB565) {
        Serial.println("[SpriteSheet] Error: Failed to allocate RGB565 palette memory");
        _lastError = AnimationError::OUT_OF_MEMORY;
        return false;
    }

    // Convert each palette entry from RGBA to RGB565 with pre-applied alpha blending
    for (int i = 0; i < 256; i++) {
        const RGBAColor& color = _palette[i];

        // Pre-apply alpha blending with black background (0x000000)
        // This eliminates per-pixel alpha blending during rendering
        uint8_t r = (color.r * color.a) / 255;
        uint8_t g = (color.g * color.a) / 255;
        uint8_t b = (color.b * color.a) / 255;

        // Convert to RGB565 format
        uint16_t r5 = (r >> 3) & 0x1F;  // 5 bits red
        uint16_t g6 = (g >> 2) & 0x3F;  // 6 bits green
        uint16_t b5 = (b >> 3) & 0x1F;  // 5 bits blue

        _paletteRGB565[i] = (r5 << 11) | (g6 << 5) | b5;
    }

    _memoryUsed += rgb565Size;

    Serial.println("[SpriteSheet] ✓ Palette loaded and converted to RGB565");
    return true;
}

bool SpriteSheet::loadFrames(const uint8_t* data, size_t size) {
    Serial.printf("[SpriteSheet] Loading %d frames (%zu bytes)...\n",
                 _header.frameCount, size);

    // Allocate frame data in PSRAM
    _frameData = (uint8_t*)allocatePSRAM(size);
    if (!_frameData) {
        Serial.println("[SpriteSheet] Error: Failed to allocate frame data memory");
        _lastError = AnimationError::OUT_OF_MEMORY;
        freeMemory();
        return false;
    }

    // Copy frame data
    memcpy(_frameData, data, size);
    _memoryUsed += size;

    // Allocate frame metadata
    size_t metadataSize = _header.frameCount * sizeof(FrameMetadata);
    _frameMetadata = (FrameMetadata*)allocatePSRAM(metadataSize);
    if (!_frameMetadata) {
        Serial.println("[SpriteSheet] Error: Failed to allocate metadata memory");
        _lastError = AnimationError::OUT_OF_MEMORY;
        freeMemory();
        return false;
    }

    // Initialize metadata (all frames are same size for uncompressed)
    for (uint16_t i = 0; i < _header.frameCount; i++) {
        _frameMetadata[i].dataOffset = i * _singleFrameSize;
        _frameMetadata[i].dataSize = _singleFrameSize;
        _frameMetadata[i].durationMs = fpsToInterval(_header.fps);
        _frameMetadata[i].isKeyFrame = true;
    }

    _memoryUsed += metadataSize;

    Serial.printf("[SpriteSheet] ✓ Frames loaded (%zu KB total)\n", _memoryUsed / 1024);
    return true;
}

void* SpriteSheet::allocatePSRAM(size_t size) {
    // Allocate in PSRAM (external RAM)
    void* ptr = heap_caps_malloc(size, MALLOC_CAP_SPIRAM);

    if (!ptr) {
        Serial.printf("[SpriteSheet] Error: PSRAM allocation failed (%zu bytes)\n", size);
    }

    return ptr;
}

void SpriteSheet::freeMemory() {
    if (_palette) {
        heap_caps_free(_palette);
        _palette = nullptr;
    }

    if (_paletteRGB565) {
        heap_caps_free(_paletteRGB565);
        _paletteRGB565 = nullptr;
    }

    if (_frameData) {
        heap_caps_free(_frameData);
        _frameData = nullptr;
    }

    if (_frameMetadata) {
        heap_caps_free(_frameMetadata);
        _frameMetadata = nullptr;
    }
}

size_t SpriteSheet::calculateFrameOffset(uint16_t frameIndex) const {
    // For uncompressed data, frames are sequential
    return frameIndex * _singleFrameSize;
}

} // namespace Animation
} // namespace Doki
