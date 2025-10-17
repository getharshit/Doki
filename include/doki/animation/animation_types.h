/**
 * @file animation_types.h
 * @brief Core animation types, enums, and structures for Doki OS
 *
 * Defines the data structures used throughout the animation system.
 * Follows the configuration pattern established in hardware_config.h
 */

#ifndef DOKI_ANIMATION_TYPES_H
#define DOKI_ANIMATION_TYPES_H

#include <Arduino.h>
#include <lvgl.h>
#include "hardware_config.h"
#include "timing_constants.h"

namespace Doki {
namespace Animation {

// ==========================================
// Constants
// ==========================================

// Magic number for sprite file format validation
constexpr uint32_t SPRITE_MAGIC = 0x444F4B49;  // "DOKI" in ASCII

// Sprite format version
constexpr uint16_t SPRITE_VERSION = 1;

// Maximum dimensions
constexpr uint16_t MAX_SPRITE_WIDTH = DISPLAY_WIDTH;
constexpr uint16_t MAX_SPRITE_HEIGHT = DISPLAY_HEIGHT;

// Maximum frame count per animation
constexpr uint16_t MAX_FRAMES_PER_ANIMATION = 120;  // 4 seconds at 30 FPS

// Header size
constexpr size_t SPRITE_HEADER_SIZE = 64;

// Palette size for 8-bit indexed color
constexpr size_t PALETTE_SIZE = 1024;  // 256 colors × 4 bytes (RGBA)

// ==========================================
// Enums
// ==========================================

/**
 * Animation playback state
 */
enum class AnimationState {
    IDLE,           // Not loaded or stopped
    LOADING,        // Loading from file/network
    LOADED,         // Loaded, ready to play
    PLAYING,        // Currently playing
    PAUSED,         // Playback paused
    ERROR           // Error state
};

/**
 * Color format for sprite data
 */
enum class ColorFormat : uint8_t {
    INDEXED_8BIT = 0,   // 8-bit indexed color (256 colors)
    RGB565 = 1,         // 16-bit RGB565
    RGB888 = 2          // 24-bit RGB888 (future)
};

/**
 * Compression format for sprite data
 */
enum class CompressionFormat : uint8_t {
    NONE = 0,           // No compression
    RLE = 1,            // Run-Length Encoding
    LZ4 = 2             // LZ4 compression (future)
};

/**
 * Animation loop mode
 */
enum class LoopMode : uint8_t {
    ONCE = 0,           // Play once and stop
    LOOP = 1,           // Loop continuously
    PING_PONG = 2       // Play forward, then backward
};

// ==========================================
// Structures
// ==========================================

/**
 * Sprite sheet file header (64 bytes)
 * First block of a .spr file
 */
struct SpriteHeader {
    uint32_t magic;                 // Magic number "DOKI" (0x444F4B49)
    uint16_t version;               // Format version
    uint16_t frameCount;            // Number of frames
    uint16_t frameWidth;            // Width of each frame in pixels
    uint16_t frameHeight;           // Height of each frame in pixels
    uint8_t fps;                    // Frames per second (1-60)
    ColorFormat colorFormat;        // Color format
    CompressionFormat compression;  // Compression format
    uint8_t reserved[49];           // Reserved for future use
} __attribute__((packed));

/**
 * RGBA color for palette
 */
struct RGBAColor {
    uint8_t r;
    uint8_t g;
    uint8_t b;
    uint8_t a;
} __attribute__((packed));

/**
 * Animation position and transform
 */
struct AnimationTransform {
    int16_t x;              // X position
    int16_t y;              // Y position
    uint16_t width;         // Display width (0 = original)
    uint16_t height;        // Display height (0 = original)
    float scaleX;           // X scale factor
    float scaleY;           // Y scale factor
    uint16_t rotation;      // Rotation in degrees (0-359)
    uint8_t opacity;        // Opacity (0-255)

    AnimationTransform()
        : x(0), y(0), width(0), height(0),
          scaleX(1.0f), scaleY(1.0f),
          rotation(0), opacity(255) {}
};

/**
 * Animation playback options
 */
struct AnimationOptions {
    LoopMode loopMode;      // Loop behavior
    float speed;            // Playback speed multiplier (0.1 - 10.0)
    bool autoPlay;          // Start playing immediately after load
    bool preload;           // Preload all frames (vs load on-demand)
    uint8_t displayId;      // Target display ID (0-2)

    AnimationOptions()
        : loopMode(LoopMode::ONCE),
          speed(1.0f),
          autoPlay(false),
          preload(true),
          displayId(0) {}
};

/**
 * Animation statistics for monitoring
 */
struct AnimationStats {
    uint32_t loadTimeMs;        // Time to load (milliseconds)
    uint32_t memoryUsed;        // Memory used (bytes)
    uint16_t framesPlayed;      // Total frames played
    uint16_t framesDropped;     // Frames dropped due to lag
    float avgFps;               // Average FPS achieved
    uint32_t lastUpdateMs;      // Last update timestamp

    AnimationStats()
        : loadTimeMs(0), memoryUsed(0),
          framesPlayed(0), framesDropped(0),
          avgFps(0.0f), lastUpdateMs(0) {}
};

/**
 * Frame metadata for tracking
 */
struct FrameMetadata {
    uint32_t dataOffset;        // Offset in sprite data
    uint32_t dataSize;          // Size of frame data (bytes)
    uint16_t durationMs;        // Frame duration (0 = use default FPS)
    bool isKeyFrame;            // True if independent frame (vs delta)

    FrameMetadata()
        : dataOffset(0), dataSize(0),
          durationMs(0), isKeyFrame(true) {}
};

// ==========================================
// Error Codes
// ==========================================

enum class AnimationError {
    NONE = 0,
    FILE_NOT_FOUND,
    INVALID_FORMAT,
    UNSUPPORTED_VERSION,
    CORRUPT_DATA,
    OUT_OF_MEMORY,
    INVALID_DIMENSIONS,
    INVALID_FRAME_COUNT,
    NETWORK_ERROR,
    TIMEOUT,
    ALREADY_LOADED,
    NOT_LOADED,
    PLAYBACK_ERROR
};

/**
 * Convert error code to string
 */
inline const char* errorToString(AnimationError error) {
    switch (error) {
        case AnimationError::NONE: return "No error";
        case AnimationError::FILE_NOT_FOUND: return "File not found";
        case AnimationError::INVALID_FORMAT: return "Invalid format";
        case AnimationError::UNSUPPORTED_VERSION: return "Unsupported version";
        case AnimationError::CORRUPT_DATA: return "Corrupt data";
        case AnimationError::OUT_OF_MEMORY: return "Out of memory";
        case AnimationError::INVALID_DIMENSIONS: return "Invalid dimensions";
        case AnimationError::INVALID_FRAME_COUNT: return "Invalid frame count";
        case AnimationError::NETWORK_ERROR: return "Network error";
        case AnimationError::TIMEOUT: return "Timeout";
        case AnimationError::ALREADY_LOADED: return "Already loaded";
        case AnimationError::NOT_LOADED: return "Not loaded";
        case AnimationError::PLAYBACK_ERROR: return "Playback error";
        default: return "Unknown error";
    }
}

// ==========================================
// Helper Functions
// ==========================================

/**
 * Calculate memory required for animation
 */
inline size_t calculateAnimationMemory(uint16_t width, uint16_t height,
                                       uint16_t frameCount,
                                       ColorFormat format = ColorFormat::INDEXED_8BIT) {
    size_t bytesPerPixel = 1;  // 8-bit indexed
    if (format == ColorFormat::RGB565) bytesPerPixel = 2;
    if (format == ColorFormat::RGB888) bytesPerPixel = 3;

    size_t frameSize = width * height * bytesPerPixel;
    size_t totalFrameData = frameSize * frameCount;
    size_t metadata = sizeof(SpriteHeader) + PALETTE_SIZE + (frameCount * sizeof(FrameMetadata));

    return totalFrameData + metadata;
}

/**
 * Calculate FPS update interval
 */
inline uint32_t fpsToInterval(uint8_t fps) {
    if (fps == 0 || fps > 60) fps = 30;  // Default to 30 FPS
    return 1000 / fps;
}

/**
 * Validate sprite dimensions
 */
inline bool validateDimensions(uint16_t width, uint16_t height) {
    return (width > 0 && width <= MAX_SPRITE_WIDTH &&
            height > 0 && height <= MAX_SPRITE_HEIGHT);
}

/**
 * Validate frame count
 */
inline bool validateFrameCount(uint16_t count) {
    return (count > 0 && count <= MAX_FRAMES_PER_ANIMATION);
}

} // namespace Animation
} // namespace Doki

#endif // DOKI_ANIMATION_TYPES_H
