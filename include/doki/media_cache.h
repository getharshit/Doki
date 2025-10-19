/**
 * @file media_cache.h
 * @brief PSRAM-based media caching system for Doki OS
 *
 * Provides fast, reliable media storage using PSRAM with optional
 * filesystem persistence. Solves SPIFFS large file write issues by
 * keeping media in RAM and only persisting small files.
 */

#ifndef DOKI_MEDIA_CACHE_H
#define DOKI_MEDIA_CACHE_H

#include <Arduino.h>
#include <map>
#include "media_service.h"

namespace Doki {

/**
 * @brief MediaCache - PSRAM-based media storage with LRU eviction
 *
 * Architecture:
 * - Primary storage: PSRAM (fast, reliable, 1.5MB available)
 * - Secondary storage: Filesystem (optional, small files only)
 * - Eviction: Least Recently Used (LRU)
 * - Thread-safe: No (call from main loop only)
 */
class MediaCache {
public:
    /**
     * @brief Cached media information
     */
    struct CachedMedia {
        String id;              ///< Unique identifier (e.g., "d0_sprite")
        MediaType type;         ///< Media type (IMAGE, GIF, SPRITE)
        uint8_t* data;          ///< PSRAM buffer pointer (owned by cache)
        size_t size;            ///< Buffer size in bytes
        uint32_t lastAccess;    ///< Last access time (millis)
        bool isPersisted;       ///< True if successfully written to filesystem
        uint8_t displayId;      ///< Target display ID
    };

    /**
     * @brief Initialize media cache system
     * @return true if successful
     */
    static bool init();

    /**
     * @brief Load media into PSRAM cache
     *
     * @param id Unique identifier for this media
     * @param data Source data buffer (will be copied to PSRAM)
     * @param size Data size in bytes
     * @param type Media type
     * @param displayId Target display ID
     * @param tryPersist If true, attempt to write to filesystem for small files
     * @return true if cached successfully, false on error
     *
     * Note: This function takes ownership of the data and will copy it to PSRAM.
     * The original buffer can be freed after this call.
     */
    static bool loadFromMemory(const String& id,
                              const uint8_t* data,
                              size_t size,
                              MediaType type,
                              uint8_t displayId,
                              bool tryPersist = true);

    /**
     * @brief Get media from cache or filesystem
     *
     * @param id Media identifier
     * @param outSize Output: Data size
     * @param outType Output: Media type
     * @return Pointer to media data (PSRAM or heap), or nullptr if not found
     *
     * Note: For cached media, returns direct PSRAM pointer (no copy).
     * For filesystem media, allocates new buffer (caller must free).
     */
    static uint8_t* getMedia(const String& id,
                            size_t* outSize = nullptr,
                            MediaType* outType = nullptr);

    /**
     * @brief Check if media exists in cache or filesystem
     *
     * @param id Media identifier
     * @return true if media is available
     */
    static bool exists(const String& id);

    /**
     * @brief Remove media from cache
     *
     * @param id Media identifier
     * @param deleteFromFilesystem If true, also delete from filesystem
     * @return true if removed, false if not found
     */
    static bool remove(const String& id, bool deleteFromFilesystem = false);

    /**
     * @brief Get cache statistics
     *
     * @param totalSize Output: Total bytes used in cache
     * @param numEntries Output: Number of cached items
     * @param numPersisted Output: Number of items also persisted to filesystem
     */
    static void getStats(size_t* totalSize = nullptr,
                        size_t* numEntries = nullptr,
                        size_t* numPersisted = nullptr);

    /**
     * @brief Clear all cache entries
     *
     * @param deleteFromFilesystem If true, also delete persisted files
     */
    static void clear(bool deleteFromFilesystem = false);

    /**
     * @brief Get maximum cache size
     * @return Maximum cache size in bytes (1.5MB)
     */
    static constexpr size_t getMaxCacheSize() { return MAX_CACHE_SIZE; }

    /**
     * @brief Get persistence threshold
     * @return Files smaller than this will attempt filesystem persistence
     */
    static constexpr size_t getPersistenceThreshold() { return PERSISTENCE_THRESHOLD; }

private:
    static const size_t MAX_CACHE_SIZE = 1536 * 1024;     ///< 1.5MB PSRAM cache
    static const size_t PERSISTENCE_THRESHOLD = 50 * 1024; ///< Files <50KB persist

    static std::map<String, CachedMedia> _cache;  ///< Active cache entries
    static size_t _totalCacheSize;                ///< Current cache usage

    /**
     * @brief Evict least recently used cache entry
     * @return true if evicted, false if cache is empty
     */
    static bool evictLRU();

    /**
     * @brief Attempt to persist media to filesystem
     *
     * @param id Media identifier
     * @param data Media data
     * @param size Data size
     * @param type Media type
     * @param displayId Display ID
     * @return true if persisted successfully
     *
     * Note: Uses chunked writes for SPIFFS compatibility.
     * Only attempts for files < PERSISTENCE_THRESHOLD.
     */
    static bool tryPersist(const String& id,
                          const uint8_t* data,
                          size_t size,
                          MediaType type,
                          uint8_t displayId);

    /**
     * @brief Generate filesystem path for media
     *
     * @param displayId Display ID
     * @param type Media type
     * @return Filesystem path (e.g., "/media/d0_anim.spr")
     */
    static String getFilesystemPath(uint8_t displayId, MediaType type);

    /**
     * @brief Make room in cache for new entry
     *
     * @param requiredSize Size needed in bytes
     * @return true if space available or freed, false if insufficient
     */
    static bool ensureSpace(size_t requiredSize);
};

} // namespace Doki

#endif // DOKI_MEDIA_CACHE_H
