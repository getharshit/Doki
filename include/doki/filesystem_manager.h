/**
 * @file filesystem_manager.h
 * @brief Filesystem management for Doki OS
 *
 * Handles initialization, file operations, and storage management.
 * Supports both SPIFFS and LittleFS with compile-time switching.
 */

#ifndef FILESYSTEM_MANAGER_H
#define FILESYSTEM_MANAGER_H

#include <Arduino.h>
#include <vector>
#include "FS.h"

// Filesystem selection (compile-time)
#ifdef USE_LITTLEFS
    #include <FS.h>
    #include <LittleFS.h>
    #define DOKI_FS LittleFS
    #define DOKI_FS_NAME "LittleFS"
#else
    #include <FS.h>
    #include <SPIFFS.h>
    #define DOKI_FS SPIFFS
    #define DOKI_FS_NAME "SPIFFS"
#endif

namespace Doki {

/**
 * @brief Filesystem Manager - Handles all SPIFFS operations
 *
 * This class provides a clean interface for filesystem operations,
 * keeping SPIFFS logic separate from main application code.
 */
class FilesystemManager {
public:
    /**
     * @brief Initialize SPIFFS filesystem
     * @param formatOnFail Format filesystem if mount fails
     * @return true if successful, false otherwise
     */
    static bool init(bool formatOnFail = true);

    /**
     * @brief Get filesystem status information
     * @param totalBytes Output: Total filesystem size in bytes
     * @param usedBytes Output: Used space in bytes
     * @return true if filesystem is mounted, false otherwise
     */
    static bool getInfo(size_t& totalBytes, size_t& usedBytes);

    /**
     * @brief Create a directory
     * @param path Directory path (e.g., "/media")
     * @return true if created or already exists, false on error
     */
    static bool createDir(const String& path);

    /**
     * @brief Check if file exists
     * @param path File path
     * @return true if file exists, false otherwise
     */
    static bool exists(const String& path);

    /**
     * @brief Read file contents
     * @param path File path
     * @param data Output buffer pointer (will be allocated)
     * @param size Output: File size
     * @return true if successful, false on error
     *
     * Note: Caller must delete[] the allocated data buffer
     */
    static bool readFile(const String& path, uint8_t** data, size_t& size);

    /**
     * @brief Write data to file
     * @param path File path
     * @param data Data buffer
     * @param size Data size
     * @return true if successful, false on error
     */
    static bool writeFile(const String& path, const uint8_t* data, size_t size);

    /**
     * @brief Delete a file
     * @param path File path
     * @return true if successful, false on error
     */
    static bool deleteFile(const String& path);

    /**
     * @brief Get file size
     * @param path File path
     * @return File size in bytes, 0 if file doesn't exist
     */
    static size_t getFileSize(const String& path);

    /**
     * @brief List all files in a directory
     * @param path Directory path
     * @param files Output vector of filenames
     * @return true if successful, false on error
     */
    static bool listFiles(const String& path, std::vector<String>& files);

    /**
     * @brief Format the filesystem (WARNING: Deletes all data)
     * @return true if successful, false on error
     */
    static bool format();

    /**
     * @brief Check if filesystem is mounted
     * @return true if mounted, false otherwise
     */
    static bool isMounted();

private:
    static bool _mounted;
};

} // namespace Doki

#endif // FILESYSTEM_MANAGER_H
