/**
 * @file media_service.cpp
 * @brief Implementation of media file management service
 */

#include "doki/media_service.h"

namespace Doki {

// Static member initialization
const String MediaService::MEDIA_DIR = "/media";

bool MediaService::init() {
    Serial.println("[MediaService] Initializing...");

    // Ensure filesystem is mounted
    if (!FilesystemManager::isMounted()) {
        Serial.println("[MediaService] Error: Filesystem not mounted");
        return false;
    }

    // Create media directory
    if (!FilesystemManager::createDir(MEDIA_DIR)) {
        Serial.println("[MediaService] Error: Failed to create media directory");
        return false;
    }

    Serial.println("[MediaService] ✓ Initialized successfully");
    return true;
}

bool MediaService::saveMedia(uint8_t displayId, const uint8_t* data, size_t size, MediaType type) {
    Serial.printf("[MediaService] Saving media for display %d (type=%d, size=%zu bytes)\n",
                  displayId, (int)type, size);

    // Validate display ID
    if (displayId > 1) {
        Serial.printf("[MediaService] Error: Invalid display ID: %d\n", displayId);
        return false;
    }

    // Validate data
    if (data == nullptr || size == 0) {
        Serial.println("[MediaService] Error: Invalid data or size");
        return false;
    }

    // Check file size limit
    if (size > MAX_FILE_SIZE) {
        Serial.printf("[MediaService] Error: File too large (%zu bytes, max %zu bytes)\n",
                      size, MAX_FILE_SIZE);
        return false;
    }

    // Detect and validate media type
    MediaType detectedType = detectMediaType(data, size);
    if (detectedType == MediaType::UNKNOWN) {
        Serial.println("[MediaService] Error: Unknown or invalid media format");
        return false;
    }

    // If type was specified, verify it matches
    if (type != MediaType::UNKNOWN && type != detectedType) {
        Serial.printf("[MediaService] Warning: Type mismatch (expected=%d, detected=%d)\n",
                      (int)type, (int)detectedType);
    }
    type = detectedType; // Use detected type

    // Validate media
    if (!validateMedia(data, size, type)) {
        Serial.println("[MediaService] Error: Media validation failed");
        return false;
    }

    // Get file path
    String path = getMediaPath(displayId, type);

    // Delete old media of same type if exists
    if (hasMedia(displayId, type)) {
        Serial.printf("[MediaService] Replacing existing media: %s\n", path.c_str());
        deleteMedia(displayId, type);
    }

    // Save to filesystem
    if (!FilesystemManager::writeFile(path, data, size)) {
        Serial.println("[MediaService] Error: Failed to write media file");
        return false;
    }

    Serial.printf("[MediaService] ✓ Media saved successfully: %s\n", path.c_str());
    return true;
}

MediaInfo MediaService::getMediaInfo(uint8_t displayId, MediaType type) {
    MediaInfo info;
    info.exists = false;
    info.type = MediaType::UNKNOWN;
    info.fileSize = 0;
    info.width = 0;
    info.height = 0;

    // Validate display ID
    if (displayId > 1) {
        return info;
    }

    String path = getMediaPath(displayId, type);
    info.path = path;

    // Check if file exists
    if (!FilesystemManager::exists(path)) {
        return info;
    }

    info.exists = true;
    info.type = type;
    info.fileSize = FilesystemManager::getFileSize(path);

    // TODO: Parse image dimensions from file headers
    // For now, dimensions are unknown (0)

    return info;
}

bool MediaService::hasMedia(uint8_t displayId, MediaType type) {
    if (displayId > 1 || type == MediaType::UNKNOWN) {
        return false;
    }

    String path = getMediaPath(displayId, type);
    return FilesystemManager::exists(path);
}

bool MediaService::deleteMedia(uint8_t displayId, MediaType type) {
    Serial.printf("[MediaService] Deleting media for display %d (type=%d)\n", displayId, (int)type);

    if (displayId > 1 || type == MediaType::UNKNOWN) {
        Serial.println("[MediaService] Error: Invalid parameters");
        return false;
    }

    String path = getMediaPath(displayId, type);

    if (!FilesystemManager::deleteFile(path)) {
        Serial.println("[MediaService] Error: Failed to delete media file");
        return false;
    }

    Serial.printf("[MediaService] ✓ Media deleted: %s\n", path.c_str());
    return true;
}

String MediaService::getMediaPath(uint8_t displayId, MediaType type) {
    String filename = MEDIA_DIR + "/d" + String(displayId);

    switch (type) {
        case MediaType::IMAGE_PNG:
            return filename + "_image.png";
        case MediaType::IMAGE_JPEG:
            return filename + "_image.jpg";
        case MediaType::GIF:
            return filename + "_anim.gif";
        case MediaType::SPRITE:
            return filename + "_anim.spr";
        default:
            return filename + "_unknown";
    }
}

MediaType MediaService::detectMediaType(const uint8_t* data, size_t size) {
    if (data == nullptr || size < 4) {
        return MediaType::UNKNOWN;
    }

    // Check sprite signature (0x444F4B49 = "DOKI" in little-endian)
    if (size >= 4) {
        uint32_t magic = *((const uint32_t*)data);

        // DEBUG: Print magic bytes
        Serial.printf("[MediaService] DEBUG: First 4 bytes = 0x%08X (expected 0x444F4B49 for SPRITE)\n", magic);

        if (magic == 0x444F4B49) {
            Serial.println("[MediaService] ✓ Detected SPRITE format");
            return MediaType::SPRITE;
        }
    }

    // Need at least 12 bytes for other formats
    if (size < 12) {
        return MediaType::UNKNOWN;
    }

    // Check PNG signature
    if (isPNG(data, size)) {
        return MediaType::IMAGE_PNG;
    }

    // Check JPEG signature
    if (isJPEG(data, size)) {
        return MediaType::IMAGE_JPEG;
    }

    // Check GIF signature
    if (isGIF(data, size)) {
        return MediaType::GIF;
    }

    return MediaType::UNKNOWN;
}

bool MediaService::validateMedia(const uint8_t* data, size_t size, MediaType type) {
    if (data == nullptr || size == 0) {
        return false;
    }

    // Check size limit
    if (size > MAX_FILE_SIZE) {
        Serial.printf("[MediaService] Validation failed: File too large (%zu bytes)\n", size);
        return false;
    }

    // Verify media type matches signature
    MediaType detected = detectMediaType(data, size);
    if (detected != type) {
        Serial.printf("[MediaService] Validation failed: Type mismatch (expected=%d, detected=%d)\n",
                      (int)type, (int)detected);
        return false;
    }

    // TODO: Add additional validation:
    // - Parse image dimensions and check against MAX_WIDTH/MAX_HEIGHT
    // - Validate image structure (not corrupted)
    // - For GIFs, check frame count and duration

    return true;
}

size_t MediaService::getTotalMediaSize() {
    size_t total = 0;

    // Check all possible media files
    for (uint8_t display = 0; display < 2; display++) {
        for (int typeInt = (int)MediaType::IMAGE_PNG; typeInt <= (int)MediaType::GIF; typeInt++) {
            MediaType type = (MediaType)typeInt;
            if (hasMedia(display, type)) {
                total += FilesystemManager::getFileSize(getMediaPath(display, type));
            }
        }
    }

    return total;
}

String MediaService::getExtension(MediaType type) {
    switch (type) {
        case MediaType::IMAGE_PNG:
            return ".png";
        case MediaType::IMAGE_JPEG:
            return ".jpg";
        case MediaType::GIF:
            return ".gif";
        case MediaType::SPRITE:
            return ".spr";
        default:
            return ".bin";
    }
}

bool MediaService::isPNG(const uint8_t* data, size_t size) {
    if (size < 8) return false;

    // PNG signature: 89 50 4E 47 0D 0A 1A 0A
    return (data[0] == 0x89 &&
            data[1] == 0x50 &&
            data[2] == 0x4E &&
            data[3] == 0x47 &&
            data[4] == 0x0D &&
            data[5] == 0x0A &&
            data[6] == 0x1A &&
            data[7] == 0x0A);
}

bool MediaService::isJPEG(const uint8_t* data, size_t size) {
    if (size < 3) return false;

    // JPEG signature: FF D8 FF
    return (data[0] == 0xFF &&
            data[1] == 0xD8 &&
            data[2] == 0xFF);
}

bool MediaService::isGIF(const uint8_t* data, size_t size) {
    if (size < 6) return false;

    // GIF signature: "GIF87a" or "GIF89a"
    return (data[0] == 'G' &&
            data[1] == 'I' &&
            data[2] == 'F' &&
            data[3] == '8' &&
            (data[4] == '7' || data[4] == '9') &&
            data[5] == 'a');
}

} // namespace Doki
