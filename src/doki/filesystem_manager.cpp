/**
 * @file filesystem_manager.cpp
 * @brief Implementation of filesystem manager (SPIFFS/LittleFS)
 */

#include "doki/filesystem_manager.h"

namespace Doki {

// Static member initialization
bool FilesystemManager::_mounted = false;

bool FilesystemManager::init(bool formatOnFail) {
    Serial.printf("\n[FilesystemManager] Initializing %s...\n", DOKI_FS_NAME);

    if (_mounted) {
        Serial.println("[FilesystemManager] Already mounted");
        return true;
    }

#ifdef USE_LITTLEFS
    // LittleFS: Always pass true to format on fail
    if (!DOKI_FS.begin(true)) {  // Auto-format if mount fails
        Serial.println("[FilesystemManager] ✗ LittleFS mount/format failed");
        return false;
    }
#else
    // SPIFFS: Use the formatOnFail parameter
    if (!DOKI_FS.begin(false)) {
        Serial.println("[FilesystemManager] Mount failed");

        if (formatOnFail) {
            Serial.println("[FilesystemManager] Formatting filesystem...");
            if (!DOKI_FS.format()) {
                Serial.println("[FilesystemManager] ✗ Format failed");
                return false;
            }

            // Try mounting again after format
            if (!DOKI_FS.begin(true)) {
                Serial.println("[FilesystemManager] ✗ Mount failed after format");
                return false;
            }
        } else {
            return false;
        }
    }
#endif

    _mounted = true;

    // Print filesystem info
    size_t total, used;
    if (getInfo(total, used)) {
        Serial.printf("[FilesystemManager] ✓ %s mounted successfully\n", DOKI_FS_NAME);
        Serial.printf("[FilesystemManager]   Total: %zu KB\n", total / 1024);
        Serial.printf("[FilesystemManager]   Used:  %zu KB\n", used / 1024);
        Serial.printf("[FilesystemManager]   Free:  %zu KB\n", (total - used) / 1024);
    }

    return true;
}

bool FilesystemManager::getInfo(size_t& totalBytes, size_t& usedBytes) {
    if (!_mounted) return false;

    totalBytes = DOKI_FS.totalBytes();
    usedBytes = DOKI_FS.usedBytes();
    return true;
}

bool FilesystemManager::createDir(const String& path) {
    if (!_mounted) {
        Serial.println("[FilesystemManager] Error: Filesystem not mounted");
        return false;
    }

    // Check if directory already exists
    if (DOKI_FS.exists(path)) {
        return true;
    }

    // Create directory
    if (!DOKI_FS.mkdir(path)) {
        Serial.printf("[FilesystemManager] Error: Failed to create directory: %s\n", path.c_str());
        return false;
    }

    Serial.printf("[FilesystemManager] ✓ Created directory: %s\n", path.c_str());
    return true;
}

bool FilesystemManager::exists(const String& path) {
    if (!_mounted) return false;
    return DOKI_FS.exists(path);
}

bool FilesystemManager::readFile(const String& path, uint8_t** data, size_t& size) {
    if (!_mounted) {
        Serial.println("[FilesystemManager] Error: Filesystem not mounted");
        return false;
    }

    // Check if file exists
    if (!exists(path)) {
        Serial.printf("[FilesystemManager] Error: File not found: %s\n", path.c_str());
        return false;
    }

    // Open file for reading
    File file = DOKI_FS.open(path, "r");
    if (!file) {
        Serial.printf("[FilesystemManager] Error: Failed to open file: %s\n", path.c_str());
        return false;
    }

    // Get file size
    size = file.size();
    if (size == 0) {
        Serial.printf("[FilesystemManager] Warning: File is empty: %s\n", path.c_str());
        file.close();
        return false;
    }

    // Allocate buffer
    *data = new uint8_t[size];
    if (*data == nullptr) {
        Serial.printf("[FilesystemManager] Error: Failed to allocate %zu bytes\n", size);
        file.close();
        return false;
    }

    // Read file contents
    size_t bytesRead = file.read(*data, size);
    file.close();

    if (bytesRead != size) {
        Serial.printf("[FilesystemManager] Error: Read %zu bytes, expected %zu\n", bytesRead, size);
        delete[] *data;
        *data = nullptr;
        return false;
    }

    Serial.printf("[FilesystemManager] ✓ Read file: %s (%zu bytes)\n", path.c_str(), size);
    return true;
}

bool FilesystemManager::writeFile(const String& path, const uint8_t* data, size_t size) {
    if (!_mounted) {
        Serial.println("[FilesystemManager] Error: Filesystem not mounted");
        return false;
    }

    if (data == nullptr || size == 0) {
        Serial.println("[FilesystemManager] Error: Invalid data or size");
        return false;
    }

    // Check available space
    size_t total, used;
    if (getInfo(total, used)) {
        size_t available = total - used;
        if (size > available) {
            Serial.printf("[FilesystemManager] Error: Insufficient space (%zu KB needed, %zu KB available)\n",
                          size / 1024, available / 1024);
            return false;
        }
    }

    // Open file for writing
    File file = DOKI_FS.open(path, "w");
    if (!file) {
        Serial.printf("[FilesystemManager] Error: Failed to open file for writing: %s\n", path.c_str());
        return false;
    }

#ifdef USE_LITTLEFS
    // LittleFS can handle large writes directly
    size_t bytesWritten = file.write(data, size);
    file.close();

    if (bytesWritten != size) {
        Serial.printf("[FilesystemManager] Error: Wrote %zu bytes, expected %zu\n", bytesWritten, size);
        return false;
    }

    Serial.printf("[FilesystemManager] ✓ Wrote file: %s (%zu KB)\n", path.c_str(), size / 1024);
    return true;
#else
    // SPIFFS needs chunked writes with periodic flushing
    const size_t CHUNK_SIZE = 4096;
    size_t offset = 0;
    size_t totalWritten = 0;

    while (offset < size) {
        size_t chunkSize = min(CHUNK_SIZE, size - offset);
        size_t written = file.write(data + offset, chunkSize);

        if (written != chunkSize) {
            Serial.printf("[FilesystemManager] Error: Write failed at offset %zu (wrote %zu, expected %zu)\n",
                         offset, written, chunkSize);
            file.close();
            return false;
        }

        offset += written;
        totalWritten += written;

        // Flush every 40KB (10 chunks) to prevent cache overflow
        if ((totalWritten % (40 * 1024)) == 0) {
            file.flush();
        }
    }

    file.flush();
    file.close();

    Serial.printf("[FilesystemManager] ✓ Wrote file: %s (%zu KB)\n", path.c_str(), size / 1024);
    return true;
#endif
}

bool FilesystemManager::deleteFile(const String& path) {
    if (!_mounted) {
        Serial.println("[FilesystemManager] Error: Filesystem not mounted");
        return false;
    }

    if (!exists(path)) {
        Serial.printf("[FilesystemManager] Warning: File doesn't exist: %s\n", path.c_str());
        return true; // Not an error if already deleted
    }

    if (!DOKI_FS.remove(path)) {
        Serial.printf("[FilesystemManager] Error: Failed to delete file: %s\n", path.c_str());
        return false;
    }

    Serial.printf("[FilesystemManager] ✓ Deleted file: %s\n", path.c_str());
    return true;
}

size_t FilesystemManager::getFileSize(const String& path) {
    if (!_mounted || !exists(path)) return 0;

    File file = DOKI_FS.open(path, "r");
    if (!file) return 0;

    size_t size = file.size();
    file.close();
    return size;
}

bool FilesystemManager::listFiles(const String& path, std::vector<String>& files) {
    if (!_mounted) {
        Serial.println("[FilesystemManager] Error: Filesystem not mounted");
        return false;
    }

    files.clear();

    File root = DOKI_FS.open(path);
    if (!root) {
        Serial.printf("[FilesystemManager] Error: Failed to open directory: %s\n", path.c_str());
        return false;
    }

    if (!root.isDirectory()) {
        Serial.printf("[FilesystemManager] Error: Not a directory: %s\n", path.c_str());
        root.close();
        return false;
    }

    File file = root.openNextFile();
    while (file) {
        if (!file.isDirectory()) {
            files.push_back(file.name());
        }
        file = root.openNextFile();
    }

    root.close();
    Serial.printf("[FilesystemManager] ✓ Listed %zu files in: %s\n", files.size(), path.c_str());
    return true;
}

bool FilesystemManager::format() {
    Serial.println("[FilesystemManager] WARNING: Formatting filesystem (all data will be lost)");

    _mounted = false;
    if (!DOKI_FS.format()) {
        Serial.println("[FilesystemManager] ✗ Format failed");
        return false;
    }

    Serial.println("[FilesystemManager] ✓ Format successful");
    return init(false);
}

bool FilesystemManager::isMounted() {
    return _mounted;
}

} // namespace Doki
