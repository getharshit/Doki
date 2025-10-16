/**
 * @file media_service.h
 * @brief Media file management service for Doki OS
 *
 * Handles storage, retrieval, and validation of image/GIF media files
 * for each display. Provides a high-level interface for media operations.
 */

#ifndef MEDIA_SERVICE_H
#define MEDIA_SERVICE_H

#include <Arduino.h>
#include "filesystem_manager.h"

namespace Doki {

/**
 * @brief Media file types
 */
enum class MediaType {
    UNKNOWN,
    IMAGE_PNG,
    IMAGE_JPEG,
    GIF
};

/**
 * @brief Media file information
 */
struct MediaInfo {
    bool exists;           ///< Whether media file exists
    MediaType type;        ///< Media type
    String path;           ///< Full file path
    size_t fileSize;       ///< File size in bytes
    uint16_t width;        ///< Image width (0 if unknown)
    uint16_t height;       ///< Image height (0 if unknown)
};

/**
 * @brief Media Service - Manages media files for displays
 *
 * This service handles all media-related operations including:
 * - Saving uploaded media files to SPIFFS
 * - Loading media for display apps
 * - Validating file formats and sizes
 * - Managing per-display media storage
 */
class MediaService {
public:
    /**
     * @brief Initialize media service
     * Creates necessary directories in SPIFFS
     * @return true if successful, false otherwise
     */
    static bool init();

    /**
     * @brief Save media file for a specific display
     * @param displayId Display ID (0 or 1)
     * @param data Media file data
     * @param size Data size in bytes
     * @param type Media type (IMAGE or GIF)
     * @return true if successful, false otherwise
     */
    static bool saveMedia(uint8_t displayId, const uint8_t* data, size_t size, MediaType type);

    /**
     * @brief Get media information for a display
     * @param displayId Display ID (0 or 1)
     * @param type Media type to query (IMAGE or GIF)
     * @return MediaInfo structure with file details
     */
    static MediaInfo getMediaInfo(uint8_t displayId, MediaType type);

    /**
     * @brief Check if display has media of specified type
     * @param displayId Display ID (0 or 1)
     * @param type Media type (IMAGE or GIF)
     * @return true if media exists, false otherwise
     */
    static bool hasMedia(uint8_t displayId, MediaType type);

    /**
     * @brief Delete media file for a display
     * @param displayId Display ID (0 or 1)
     * @param type Media type (IMAGE or GIF)
     * @return true if successful, false otherwise
     */
    static bool deleteMedia(uint8_t displayId, MediaType type);

    /**
     * @brief Get media file path for display
     * @param displayId Display ID (0 or 1)
     * @param type Media type (IMAGE or GIF)
     * @return Full file path (e.g., "/media/d0_image.png")
     */
    static String getMediaPath(uint8_t displayId, MediaType type);

    /**
     * @brief Detect media type from file data
     * Checks magic bytes at start of file
     * @param data File data
     * @param size Data size
     * @return Detected MediaType
     */
    static MediaType detectMediaType(const uint8_t* data, size_t size);

    /**
     * @brief Validate media file
     * Checks format, size constraints, etc.
     * @param data File data
     * @param size Data size
     * @param type Expected media type
     * @return true if valid, false otherwise
     */
    static bool validateMedia(const uint8_t* data, size_t size, MediaType type);

    /**
     * @brief Get total storage used by media files
     * @return Total bytes used
     */
    static size_t getTotalMediaSize();

    // Constants
    static constexpr size_t MAX_FILE_SIZE = 1024 * 1024;  ///< 1MB max file size
    static constexpr uint16_t MAX_WIDTH = 240;            ///< Max image width
    static constexpr uint16_t MAX_HEIGHT = 320;           ///< Max image height

private:
    /**
     * @brief Get file extension for media type
     * @param type Media type
     * @return File extension (e.g., ".png", ".gif")
     */
    static String getExtension(MediaType type);

    /**
     * @brief Check PNG file signature
     * @param data File data
     * @param size Data size
     * @return true if valid PNG, false otherwise
     */
    static bool isPNG(const uint8_t* data, size_t size);

    /**
     * @brief Check JPEG file signature
     * @param data File data
     * @param size Data size
     * @return true if valid JPEG, false otherwise
     */
    static bool isJPEG(const uint8_t* data, size_t size);

    /**
     * @brief Check GIF file signature
     * @param data File data
     * @param size Data size
     * @return true if valid GIF, false otherwise
     */
    static bool isGIF(const uint8_t* data, size_t size);

    static const String MEDIA_DIR;  ///< Media directory path
};

} // namespace Doki

#endif // MEDIA_SERVICE_H
