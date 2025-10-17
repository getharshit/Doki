/**
 * @file sprite_sheet.h
 * @brief Sprite sheet loader and data manager for Doki OS Animation System
 *
 * Handles loading sprite sheet files from filesystem or memory,
 * validates format, and manages sprite data in PSRAM.
 */

#ifndef DOKI_ANIMATION_SPRITE_SHEET_H
#define DOKI_ANIMATION_SPRITE_SHEET_H

#include <Arduino.h>
#include "animation_types.h"
#include "doki/filesystem_manager.h"

namespace Doki {
namespace Animation {

/**
 * SpriteSheet class
 *
 * Loads and manages sprite sheet data (.spr files).
 * All sprite data is stored in PSRAM to preserve internal RAM.
 *
 * Usage:
 *   SpriteSheet* sprite = new SpriteSheet();
 *   if (sprite->loadFromFile("/animations/loading.spr")) {
 *       // Use sprite data
 *   }
 *   delete sprite;
 */
class SpriteSheet {
public:
    // ==========================================
    // Constructor / Destructor
    // ==========================================

    SpriteSheet();
    ~SpriteSheet();

    // Prevent copying (sprite data in PSRAM)
    SpriteSheet(const SpriteSheet&) = delete;
    SpriteSheet& operator=(const SpriteSheet&) = delete;

    // ==========================================
    // Loading Methods
    // ==========================================

    /**
     * Load sprite sheet from filesystem
     * @param filepath Path to .spr file (e.g., "/animations/sprite.spr")
     * @return true if loaded successfully, false on error
     */
    bool loadFromFile(const char* filepath);

    /**
     * Load sprite sheet from memory buffer
     * @param data Pointer to sprite data in memory
     * @param size Size of data in bytes
     * @return true if loaded successfully, false on error
     */
    bool loadFromMemory(const uint8_t* data, size_t size);

    /**
     * Unload sprite sheet and free memory
     */
    void unload();

    // ==========================================
    // Status & Info
    // ==========================================

    /**
     * Check if sprite sheet is loaded
     */
    bool isLoaded() const { return _loaded; }

    /**
     * Get last error
     */
    AnimationError getError() const { return _lastError; }

    /**
     * Get error as string
     */
    const char* getErrorString() const { return errorToString(_lastError); }

    /**
     * Get memory used by sprite sheet (bytes)
     */
    size_t getMemoryUsed() const { return _memoryUsed; }

    /**
     * Get load time (milliseconds)
     */
    uint32_t getLoadTime() const { return _loadTimeMs; }

    // ==========================================
    // Sprite Info
    // ==========================================

    /**
     * Get sprite header (metadata)
     */
    const SpriteHeader& getHeader() const { return _header; }

    /**
     * Get frame count
     */
    uint16_t getFrameCount() const { return _header.frameCount; }

    /**
     * Get frame width
     */
    uint16_t getFrameWidth() const { return _header.frameWidth; }

    /**
     * Get frame height
     */
    uint16_t getFrameHeight() const { return _header.frameHeight; }

    /**
     * Get frames per second
     */
    uint8_t getFPS() const { return _header.fps; }

    /**
     * Get color format
     */
    ColorFormat getColorFormat() const { return _header.colorFormat; }

    /**
     * Get compression format
     */
    CompressionFormat getCompression() const { return _header.compression; }

    // ==========================================
    // Frame Data Access
    // ==========================================

    /**
     * Get frame data pointer
     * @param frameIndex Frame index (0 to frameCount-1)
     * @return Pointer to frame data, or nullptr if invalid
     */
    const uint8_t* getFrameData(uint16_t frameIndex) const;

    /**
     * Get frame size in bytes
     * @param frameIndex Frame index
     * @return Size of frame data in bytes
     */
    size_t getFrameSize(uint16_t frameIndex) const;

    /**
     * Get palette data (for indexed color formats)
     * @return Pointer to palette data (256 RGBA colors), or nullptr
     */
    const RGBAColor* getPalette() const { return _palette; }

    /**
     * Get frame metadata
     * @param frameIndex Frame index
     * @return Frame metadata, or nullptr if invalid
     */
    const FrameMetadata* getFrameMetadata(uint16_t frameIndex) const;

    // ==========================================
    // Validation
    // ==========================================

    /**
     * Validate frame index
     */
    bool isValidFrame(uint16_t frameIndex) const {
        return (frameIndex < _header.frameCount);
    }

    /**
     * Print sprite info to Serial (for debugging)
     */
    void printInfo() const;

private:
    // ==========================================
    // Internal Methods
    // ==========================================

    /**
     * Parse and validate sprite data
     */
    bool parseSprite(const uint8_t* data, size_t size);

    /**
     * Validate header
     */
    bool validateHeader(const SpriteHeader& header);

    /**
     * Load palette from data
     */
    bool loadPalette(const uint8_t* data);

    /**
     * Load frame data
     */
    bool loadFrames(const uint8_t* data, size_t size);

    /**
     * Allocate memory in PSRAM
     */
    void* allocatePSRAM(size_t size);

    /**
     * Free all allocated memory
     */
    void freeMemory();

    /**
     * Calculate frame data offset
     */
    size_t calculateFrameOffset(uint16_t frameIndex) const;

    // ==========================================
    // Member Variables
    // ==========================================

    bool _loaded;                       // Is sprite loaded?
    AnimationError _lastError;          // Last error code
    uint32_t _loadTimeMs;               // Load time in milliseconds
    size_t _memoryUsed;                 // Total memory used

    SpriteHeader _header;               // Sprite header
    RGBAColor* _palette;                // Palette data (PSRAM)
    uint8_t* _frameData;                // All frame data (PSRAM)
    FrameMetadata* _frameMetadata;      // Frame metadata (PSRAM)

    size_t _frameDataSize;              // Total size of frame data
    size_t _singleFrameSize;            // Size of one frame (bytes)
};

} // namespace Animation
} // namespace Doki

#endif // DOKI_ANIMATION_SPRITE_SHEET_H
