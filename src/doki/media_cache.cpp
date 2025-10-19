/**
 * @file media_cache.cpp
 * @brief Implementation of PSRAM-based media caching system
 */

#include "doki/media_cache.h"
#include "doki/filesystem_manager.h"
#include "doki/media_service.h"
#include <esp_heap_caps.h>

namespace Doki {

// Static member initialization
std::map<String, MediaCache::CachedMedia> MediaCache::_cache;
size_t MediaCache::_totalCacheSize = 0;

bool MediaCache::init() {
    Serial.println("[MediaCache] Initializing PSRAM media cache...");

    // Check PSRAM availability
    size_t freePSRAM = heap_caps_get_free_size(MALLOC_CAP_SPIRAM);
    size_t totalPSRAM = heap_caps_get_total_size(MALLOC_CAP_SPIRAM);

    if (totalPSRAM == 0) {
        Serial.println("[MediaCache] Error: No PSRAM available!");
        return false;
    }

    Serial.printf("[MediaCache] PSRAM: %zu KB free / %zu KB total\n",
                 freePSRAM / 1024, totalPSRAM / 1024);
    Serial.printf("[MediaCache] Cache size: %zu KB\n", MAX_CACHE_SIZE / 1024);
    Serial.printf("[MediaCache] Persistence threshold: %zu KB\n",
                 PERSISTENCE_THRESHOLD / 1024);

    // Clear any existing cache
    clear(false);

    Serial.println("[MediaCache] ✓ Initialized successfully");
    return true;
}

bool MediaCache::loadFromMemory(const String& id,
                               const uint8_t* data,
                               size_t size,
                               MediaType type,
                               uint8_t displayId,
                               bool tryPersist) {
    if (data == nullptr || size == 0) {
        Serial.println("[MediaCache] Error: Invalid data");
        return false;
    }

    Serial.printf("[MediaCache] Loading '%s' (%zu KB, type=%d, display=%d)\n",
                 id.c_str(), size / 1024, (int)type, displayId);

    // Check if already cached
    auto it = _cache.find(id);
    if (it != _cache.end()) {
        Serial.printf("[MediaCache] Replacing existing cache entry for '%s'\n", id.c_str());
        _totalCacheSize -= it->second.size;
        free(it->second.data);  // Free old PSRAM buffer
        _cache.erase(it);
    }

    // Ensure we have space
    if (!ensureSpace(size)) {
        Serial.printf("[MediaCache] Error: Cannot free %zu KB for '%s'\n",
                     size / 1024, id.c_str());
        return false;
    }

    // Allocate PSRAM buffer
    uint8_t* psramBuffer = (uint8_t*)ps_malloc(size);
    if (psramBuffer == nullptr) {
        Serial.printf("[MediaCache] Error: Failed to allocate %zu KB PSRAM\n",
                     size / 1024);
        return false;
    }

    // Copy data to PSRAM
    memcpy(psramBuffer, data, size);

    // Create cache entry
    CachedMedia entry;
    entry.id = id;
    entry.type = type;
    entry.data = psramBuffer;
    entry.size = size;
    entry.lastAccess = millis();
    entry.isPersisted = false;
    entry.displayId = displayId;

    // Add to cache
    _cache[id] = entry;
    _totalCacheSize += size;

    Serial.printf("[MediaCache] ✓ Cached '%s' in PSRAM (%zu KB total used)\n",
                 id.c_str(), _totalCacheSize / 1024);

    // Try to persist small files
    if (tryPersist && size <= PERSISTENCE_THRESHOLD) {
        if (MediaCache::tryPersist(id, psramBuffer, size, type, displayId)) {
            _cache[id].isPersisted = true;
            Serial.printf("[MediaCache] ✓ Persisted '%s' to filesystem\n", id.c_str());
        } else {
            Serial.printf("[MediaCache] ⚠️ Could not persist '%s' (PSRAM only)\n", id.c_str());
        }
    } else if (size > PERSISTENCE_THRESHOLD) {
        Serial.printf("[MediaCache] File too large for persistence (%zu KB > %zu KB threshold)\n",
                     size / 1024, PERSISTENCE_THRESHOLD / 1024);
    }

    return true;
}

uint8_t* MediaCache::getMedia(const String& id, size_t* outSize, MediaType* outType) {
    // Check cache first
    auto it = _cache.find(id);
    if (it != _cache.end()) {
        // Update last access time
        it->second.lastAccess = millis();

        if (outSize) *outSize = it->second.size;
        if (outType) *outType = it->second.type;

        Serial.printf("[MediaCache] Cache hit: '%s' (%zu KB from PSRAM)\n",
                     id.c_str(), it->second.size / 1024);
        return it->second.data;
    }

    // Not in cache - try filesystem
    Serial.printf("[MediaCache] Cache miss: '%s', checking filesystem...\n", id.c_str());

    // Try to determine filesystem path
    // This is a best-effort attempt for persisted files
    for (uint8_t displayId = 0; displayId < 2; displayId++) {
        for (int typeInt = (int)MediaType::IMAGE_PNG; typeInt <= (int)MediaType::SPRITE; typeInt++) {
            MediaType type = (MediaType)typeInt;
            String path = getFilesystemPath(displayId, type);

            if (FilesystemManager::exists(path)) {
                uint8_t* data = nullptr;
                size_t size = 0;

                if (FilesystemManager::readFile(path, &data, size)) {
                    if (outSize) *outSize = size;
                    if (outType) *outType = type;

                    Serial.printf("[MediaCache] ✓ Loaded '%s' from filesystem (%zu KB)\n",
                                 id.c_str(), size / 1024);
                    return data;  // Caller must free this
                }
            }
        }
    }

    Serial.printf("[MediaCache] Not found: '%s'\n", id.c_str());
    return nullptr;
}

bool MediaCache::exists(const String& id) {
    // Check cache
    if (_cache.find(id) != _cache.end()) {
        return true;
    }

    // Check filesystem (best-effort)
    for (uint8_t displayId = 0; displayId < 2; displayId++) {
        for (int typeInt = (int)MediaType::IMAGE_PNG; typeInt <= (int)MediaType::SPRITE; typeInt++) {
            MediaType type = (MediaType)typeInt;
            String path = getFilesystemPath(displayId, type);
            if (FilesystemManager::exists(path)) {
                return true;
            }
        }
    }

    return false;
}

bool MediaCache::remove(const String& id, bool deleteFromFilesystem) {
    auto it = _cache.find(id);
    if (it == _cache.end()) {
        return false;
    }

    // Free PSRAM buffer
    _totalCacheSize -= it->second.size;
    free(it->second.data);

    // Delete from filesystem if requested
    if (deleteFromFilesystem && it->second.isPersisted) {
        String path = getFilesystemPath(it->second.displayId, it->second.type);
        FilesystemManager::deleteFile(path);
        Serial.printf("[MediaCache] Deleted '%s' from filesystem\n", id.c_str());
    }

    _cache.erase(it);
    Serial.printf("[MediaCache] Removed '%s' from cache\n", id.c_str());
    return true;
}

void MediaCache::getStats(size_t* totalSize, size_t* numEntries, size_t* numPersisted) {
    if (totalSize) *totalSize = _totalCacheSize;
    if (numEntries) *numEntries = _cache.size();

    if (numPersisted) {
        size_t count = 0;
        for (const auto& pair : _cache) {
            if (pair.second.isPersisted) count++;
        }
        *numPersisted = count;
    }
}

void MediaCache::clear(bool deleteFromFilesystem) {
    Serial.printf("[MediaCache] Clearing cache (%zu entries, %zu KB)\n",
                 _cache.size(), _totalCacheSize / 1024);

    for (auto& pair : _cache) {
        if (pair.second.data) {
            free(pair.second.data);
        }

        if (deleteFromFilesystem && pair.second.isPersisted) {
            String path = getFilesystemPath(pair.second.displayId, pair.second.type);
            FilesystemManager::deleteFile(path);
        }
    }

    _cache.clear();
    _totalCacheSize = 0;

    Serial.println("[MediaCache] ✓ Cache cleared");
}

bool MediaCache::evictLRU() {
    if (_cache.empty()) {
        return false;
    }

    // Find least recently used entry
    auto lruIt = _cache.begin();
    uint32_t oldestTime = lruIt->second.lastAccess;

    for (auto it = _cache.begin(); it != _cache.end(); ++it) {
        if (it->second.lastAccess < oldestTime) {
            oldestTime = it->second.lastAccess;
            lruIt = it;
        }
    }

    Serial.printf("[MediaCache] Evicting LRU: '%s' (%zu KB, age=%lu ms)\n",
                 lruIt->second.id.c_str(),
                 lruIt->second.size / 1024,
                 millis() - lruIt->second.lastAccess);

    // Free PSRAM buffer
    _totalCacheSize -= lruIt->second.size;
    free(lruIt->second.data);

    // Note: We keep persisted files on filesystem
    _cache.erase(lruIt);

    return true;
}

bool MediaCache::tryPersist(const String& id,
                           const uint8_t* data,
                           size_t size,
                           MediaType type,
                           uint8_t displayId) {
    String path = getFilesystemPath(displayId, type);

    Serial.printf("[MediaCache] Attempting to persist %zu KB to %s...\n",
                 size / 1024, path.c_str());

    // Check filesystem space
    size_t totalBytes, usedBytes;
    if (!FilesystemManager::getInfo(totalBytes, usedBytes)) {
        Serial.println("[MediaCache] Error: Could not get filesystem info");
        return false;
    }

    size_t freeBytes = totalBytes - usedBytes;
    if (size > freeBytes) {
        Serial.printf("[MediaCache] Insufficient space: need %zu KB, have %zu KB\n",
                     size / 1024, freeBytes / 1024);
        return false;
    }

    // Use FilesystemManager's write (which will handle chunking for SPIFFS)
    bool success = FilesystemManager::writeFile(path, data, size);

    if (success) {
        Serial.printf("[MediaCache] ✓ Persisted %zu KB to %s\n",
                     size / 1024, path.c_str());
    } else {
        Serial.printf("[MediaCache] ✗ Failed to persist to %s\n", path.c_str());
    }

    return success;
}

String MediaCache::getFilesystemPath(uint8_t displayId, MediaType type) {
    // Use MediaService to generate consistent paths
    return MediaService::getMediaPath(displayId, type);
}

bool MediaCache::ensureSpace(size_t requiredSize) {
    // Check if we have space
    if (_totalCacheSize + requiredSize <= MAX_CACHE_SIZE) {
        return true;
    }

    Serial.printf("[MediaCache] Need %zu KB, have %zu KB used / %zu KB max\n",
                 requiredSize / 1024, _totalCacheSize / 1024, MAX_CACHE_SIZE / 1024);

    // Try to evict entries until we have space
    while (_totalCacheSize + requiredSize > MAX_CACHE_SIZE) {
        if (!evictLRU()) {
            // No more entries to evict
            Serial.println("[MediaCache] Cannot evict more entries");
            return false;
        }
    }

    Serial.printf("[MediaCache] ✓ Freed space, now %zu KB used\n", _totalCacheSize / 1024);
    return true;
}

} // namespace Doki
