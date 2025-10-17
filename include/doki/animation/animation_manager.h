/**
 * @file animation_manager.h
 * @brief Central animation management singleton for Doki OS
 *
 * Manages all animations system-wide with memory pool allocation,
 * caching, and per-display support.
 */

#ifndef DOKI_ANIMATION_ANIMATION_MANAGER_H
#define DOKI_ANIMATION_ANIMATION_MANAGER_H

#include <Arduino.h>
#include <map>
#include "animation_types.h"
#include "sprite_sheet.h"
#include "animation_player.h"

namespace Doki {
namespace Animation {

/**
 * Animation cache entry
 */
struct AnimationEntry {
    SpriteSheet* sprite;            // Loaded sprite sheet
    AnimationPlayer* player;        // Active player (null if not playing)
    lv_obj_t* parent;               // Parent LVGL object (for lazy player creation)
    uint32_t lastAccessTime;        // Last access timestamp (for LRU)
    uint16_t useCount;              // Number of times used
    bool isPinned;                  // Prevent eviction if true

    AnimationEntry()
        : sprite(nullptr), player(nullptr), parent(nullptr),
          lastAccessTime(0), useCount(0), isPinned(false) {}
};

/**
 * AnimationManager - Singleton
 *
 * Central manager for all animations in Doki OS.
 * Handles loading, caching, memory management, and playback.
 *
 * Usage:
 *   AnimationManager& mgr = AnimationManager::getInstance();
 *   mgr.init();
 *   auto id = mgr.loadAnimation("/animations/loading.spr", screen);
 *   mgr.playAnimation(id);
 */
class AnimationManager {
public:
    // ==========================================
    // Singleton Access
    // ==========================================

    /**
     * Get singleton instance
     */
    static AnimationManager& getInstance() {
        static AnimationManager instance;
        return instance;
    }

    // Prevent copying
    AnimationManager(const AnimationManager&) = delete;
    AnimationManager& operator=(const AnimationManager&) = delete;

    // ==========================================
    // Initialization
    // ==========================================

    /**
     * Initialize animation system
     * @return true if initialized successfully
     */
    bool init();

    /**
     * Shutdown animation system and free all resources
     */
    void shutdown();

    /**
     * Check if initialized
     */
    bool isInitialized() const { return _initialized; }

    // ==========================================
    // Animation Loading
    // ==========================================

    /**
     * Load animation from file
     * @param filepath Path to .spr file
     * @param parent LVGL parent object for display
     * @param options Playback options
     * @return Animation ID (>= 0) or -1 on error
     */
    int32_t loadAnimation(const char* filepath, lv_obj_t* parent,
                          const AnimationOptions& options = AnimationOptions());

    /**
     * Load animation from memory
     * @param data Sprite data buffer
     * @param size Size of data
     * @param parent LVGL parent object
     * @param options Playback options
     * @return Animation ID or -1 on error
     */
    int32_t loadAnimationFromMemory(const uint8_t* data, size_t size,
                                    lv_obj_t* parent,
                                    const AnimationOptions& options = AnimationOptions());

    /**
     * Unload animation and free resources
     * @param animationId Animation ID
     */
    void unloadAnimation(int32_t animationId);

    /**
     * Unload all animations
     */
    void unloadAll();

    // ==========================================
    // Playback Control
    // ==========================================

    /**
     * Play animation
     * @param animationId Animation ID
     * @param mode Loop mode
     * @return true if started successfully
     */
    bool playAnimation(int32_t animationId, LoopMode mode = LoopMode::ONCE);

    /**
     * Pause animation
     */
    void pauseAnimation(int32_t animationId);

    /**
     * Resume animation
     */
    void resumeAnimation(int32_t animationId);

    /**
     * Stop animation
     */
    void stopAnimation(int32_t animationId);

    /**
     * Stop all animations
     */
    void stopAll();

    /**
     * Update all animations (call in main loop)
     */
    void updateAll();

    // ==========================================
    // Animation Control
    // ==========================================

    /**
     * Set animation speed
     */
    void setSpeed(int32_t animationId, float speed);

    /**
     * Set animation position
     */
    void setPosition(int32_t animationId, int16_t x, int16_t y);

    /**
     * Set animation opacity
     */
    void setOpacity(int32_t animationId, uint8_t opacity);

    /**
     * Set animation visibility
     */
    void setVisible(int32_t animationId, bool visible);

    // ==========================================
    // Status & Info
    // ==========================================

    /**
     * Check if animation ID is valid
     */
    bool isValidAnimation(int32_t animationId) const;

    /**
     * Get animation player (for direct control)
     */
    AnimationPlayer* getPlayer(int32_t animationId);

    /**
     * Get animation state
     */
    AnimationState getState(int32_t animationId) const;

    /**
     * Get number of loaded animations
     */
    size_t getLoadedCount() const { return _animations.size(); }

    /**
     * Get total memory used by animations
     */
    size_t getMemoryUsed() const { return _memoryUsed; }

    /**
     * Get available memory in pool
     */
    size_t getMemoryAvailable() const {
        size_t total = ANIMATION_POOL_SIZE_KB * 1024;
        return (total > _memoryUsed) ? (total - _memoryUsed) : 0;
    }

    // ==========================================
    // Cache Management
    // ==========================================

    /**
     * Pin animation in cache (prevent eviction)
     */
    void pinAnimation(int32_t animationId);

    /**
     * Unpin animation
     */
    void unpinAnimation(int32_t animationId);

    /**
     * Clear cache (unload unused animations)
     */
    void clearCache();

    /**
     * Set cache size limit (bytes)
     */
    void setCacheSizeLimit(size_t limit) { _cacheSizeLimit = limit; }

    // ==========================================
    // Statistics
    // ==========================================

    /**
     * Get system-wide statistics
     */
    struct SystemStats {
        size_t loadedCount;         // Number of loaded animations
        size_t playingCount;        // Number of playing animations
        size_t memoryUsed;          // Total memory used (bytes)
        size_t memoryAvailable;     // Available memory (bytes)
        uint32_t totalLoads;        // Total animations loaded (lifetime)
        uint32_t cacheHits;         // Cache hits
        uint32_t cacheMisses;       // Cache misses
        uint32_t evictions;         // Cache evictions
    };

    SystemStats getSystemStats() const;

    /**
     * Print system info (for debugging)
     */
    void printSystemInfo() const;

private:
    // ==========================================
    // Constructor (private for singleton)
    // ==========================================

    AnimationManager();
    ~AnimationManager();

    // ==========================================
    // Internal Methods
    // ==========================================

    /**
     * Generate unique animation ID
     */
    int32_t generateId();

    /**
     * Get animation entry
     */
    AnimationEntry* getEntry(int32_t animationId);
    const AnimationEntry* getEntry(int32_t animationId) const;

    /**
     * Check if memory is available
     */
    bool hasMemoryAvailable(size_t required);

    /**
     * Evict least recently used animation
     */
    bool evictLRU();

    /**
     * Update memory usage
     */
    void updateMemoryUsage();

    // ==========================================
    // Member Variables
    // ==========================================

    bool _initialized;                              // Initialization flag
    int32_t _nextId;                                // Next animation ID

    std::map<int32_t, AnimationEntry> _animations;  // All loaded animations
    size_t _memoryUsed;                             // Total memory used
    size_t _cacheSizeLimit;                         // Cache size limit

    // Statistics
    uint32_t _totalLoads;
    uint32_t _cacheHits;
    uint32_t _cacheMisses;
    uint32_t _evictions;
};

} // namespace Animation
} // namespace Doki

#endif // DOKI_ANIMATION_ANIMATION_MANAGER_H
