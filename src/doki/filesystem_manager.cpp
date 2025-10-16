/**
 * @file filesystem_manager.cpp
 * @brief Implementation of SPIFFS filesystem manager
 */

#include "doki/filesystem_manager.h"

namespace Doki {

// Static member initialization
bool FilesystemManager::_mounted = false;

bool FilesystemManager::init(bool formatOnFail) {
    Serial.println("\n[FilesystemManager] Initializing SPIFFS...");

    if (_mounted) {
        Serial.println("[FilesystemManager] Already mounted");
        return true;
    }

    // Try to mount SPIFFS
    if (!SPIFFS.begin(false)) {
        Serial.println("[FilesystemManager] Mount failed");

        if (formatOnFail) {
            Serial.println("[FilesystemManager] Formatting filesystem...");
            if (!SPIFFS.format()) {
                Serial.println("[FilesystemManager] ✗ Format failed");
                return false;
            }

            // Try mounting again after format
            if (!SPIFFS.begin(true)) {
                Serial.println("[FilesystemManager] ✗ Mount failed after format");
                return false;
            }
        } else {
            return false;
        }
    }

    _mounted = true;

    // Print filesystem info
    size_t total, used;
    if (getInfo(total, used)) {
        Serial.printf("[FilesystemManager] ✓ Mounted successfully\n");
        Serial.printf("[FilesystemManager]   Total: %zu KB\n", total / 1024);
        Serial.printf("[FilesystemManager]   Used:  %zu KB\n", used / 1024);
        Serial.printf("[FilesystemManager]   Free:  %zu KB\n", (total - used) / 1024);
    }

    return true;
}

bool FilesystemManager::getInfo(size_t& totalBytes, size_t& usedBytes) {
    if (!_mounted) return false;

    totalBytes = SPIFFS.totalBytes();
    usedBytes = SPIFFS.usedBytes();
    return true;
}

bool FilesystemManager::createDir(const String& path) {
    if (!_mounted) {
        Serial.println("[FilesystemManager] Error: Filesystem not mounted");
        return false;
    }

    // Check if directory already exists
    if (SPIFFS.exists(path)) {
        return true;
    }

    // Create directory
    if (!SPIFFS.mkdir(path)) {
        Serial.printf("[FilesystemManager] Error: Failed to create directory: %s\n", path.c_str());
        return false;
    }

    Serial.printf("[FilesystemManager] ✓ Created directory: %s\n", path.c_str());
    return true;
}

bool FilesystemManager::exists(const String& path) {
    if (!_mounted) return false;
    return SPIFFS.exists(path);
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
    File file = SPIFFS.open(path, "r");
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
    File file = SPIFFS.open(path, "w");
    if (!file) {
        Serial.printf("[FilesystemManager] Error: Failed to open file for writing: %s\n", path.c_str());
        return false;
    }

    // Write data
    size_t bytesWritten = file.write(data, size);
    file.close();

    if (bytesWritten != size) {
        Serial.printf("[FilesystemManager] Error: Wrote %zu bytes, expected %zu\n", bytesWritten, size);
        return false;
    }

    Serial.printf("[FilesystemManager] ✓ Wrote file: %s (%zu bytes)\n", path.c_str(), size);
    return true;
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

    if (!SPIFFS.remove(path)) {
        Serial.printf("[FilesystemManager] Error: Failed to delete file: %s\n", path.c_str());
        return false;
    }

    Serial.printf("[FilesystemManager] ✓ Deleted file: %s\n", path.c_str());
    return true;
}

size_t FilesystemManager::getFileSize(const String& path) {
    if (!_mounted || !exists(path)) return 0;

    File file = SPIFFS.open(path, "r");
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

    File root = SPIFFS.open(path);
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
    if (!SPIFFS.format()) {
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
