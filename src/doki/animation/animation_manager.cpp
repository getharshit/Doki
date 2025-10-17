/**
 * @file animation_manager.cpp
 * @brief Implementation of AnimationManager singleton
 */

#include "doki/animation/animation_manager.h"

namespace Doki {
namespace Animation {

// ==========================================
// Constructor / Destructor
// ==========================================

AnimationManager::AnimationManager()
    : _initialized(false),
      _nextId(1),
      _memoryUsed(0),
      _cacheSizeLimit(ANIMATION_POOL_SIZE_KB * 1024),
      _totalLoads(0),
      _cacheHits(0),
      _cacheMisses(0),
      _evictions(0) {
}

AnimationManager::~AnimationManager() {
    shutdown();
}

// ==========================================
// Initialization
// ==========================================

bool AnimationManager::init() {
    if (_initialized) {
        Serial.println("[AnimationManager] Already initialized");
        return true;
    }

    Serial.println("[AnimationManager] Initializing...");

    // Check PSRAM availability
    size_t psramSize = ESP.getPsramSize();
    size_t psramFree = ESP.getFreePsram();

    Serial.printf("[AnimationManager] PSRAM: %zu bytes total, %zu bytes free\n",
                 psramSize, psramFree);

    if (psramSize == 0) {
        Serial.println("[AnimationManager] Error: PSRAM not available");
        return false;
    }

    size_t poolSize = ANIMATION_POOL_SIZE_KB * 1024;
    if (psramFree < poolSize) {
        Serial.printf("[AnimationManager] Warning: Limited PSRAM (need %zu, have %zu)\n",
                     poolSize, psramFree);
    }

    _initialized = true;
    Serial.printf("[AnimationManager] ✓ Initialized (pool: %zu KB)\n",
                 ANIMATION_POOL_SIZE_KB);

    return true;
}

void AnimationManager::shutdown() {
    if (!_initialized) {
        return;
    }

    Serial.println("[AnimationManager] Shutting down...");

    unloadAll();

    _initialized = false;
    Serial.println("[AnimationManager] ✓ Shutdown complete");
}

// ==========================================
// Animation Loading
// ==========================================

int32_t AnimationManager::loadAnimation(const char* filepath, lv_obj_t* parent,
                                        const AnimationOptions& options) {
    if (!_initialized) {
        Serial.println("[AnimationManager] Error: Not initialized");
        return -1;
    }

    Serial.printf("[AnimationManager] Loading animation: %s\n", filepath);

    // Create sprite sheet
    SpriteSheet* sprite = new SpriteSheet();
    if (!sprite) {
        Serial.println("[AnimationManager] Error: Failed to create sprite");
        return -1;
    }

    // Load sprite
    if (!sprite->loadFromFile(filepath)) {
        Serial.printf("[AnimationManager] Error: Failed to load sprite: %s\n",
                     sprite->getErrorString());
        delete sprite;
        return -1;
    }

    // Check memory availability
    size_t required = sprite->getMemoryUsed();
    if (!hasMemoryAvailable(required)) {
        Serial.printf("[AnimationManager] Error: Insufficient memory (%zu required)\n",
                     required);

        // Try to evict LRU
        if (evictLRU()) {
            Serial.println("[AnimationManager] Evicted LRU, retrying...");
            if (!hasMemoryAvailable(required)) {
                delete sprite;
                return -1;
            }
        } else {
            delete sprite;
            return -1;
        }
    }

    // Create player if autoPlay is enabled
    AnimationPlayer* player = nullptr;
    if (options.autoPlay) {
        player = new AnimationPlayer(sprite, parent);
        if (!player || player->getState() == AnimationState::ERROR) {
            Serial.println("[AnimationManager] Error: Failed to create player");
            delete player;
            delete sprite;
            return -1;
        }

        player->setPosition(0, 0);  // Default position
        required += player->getStats().memoryUsed;
    }

    // Generate ID and create entry
    int32_t id = generateId();
    AnimationEntry entry;
    entry.sprite = sprite;
    entry.player = player;
    entry.parent = parent;  // Store parent for lazy player creation
    entry.lastAccessTime = millis();
    entry.useCount = 1;
    entry.isPinned = false;

    _animations[id] = entry;
    _memoryUsed += required;
    _totalLoads++;
    _cacheMisses++;

    Serial.printf("[AnimationManager] ✓ Loaded animation #%d (%zu KB used)\n",
                 id, _memoryUsed / 1024);

    // Auto-play if requested
    if (options.autoPlay && player) {
        player->play(options.loopMode);
    }

    return id;
}

int32_t AnimationManager::loadAnimationFromMemory(const uint8_t* data, size_t size,
                                                  lv_obj_t* parent,
                                                  const AnimationOptions& options) {
    if (!_initialized) {
        Serial.println("[AnimationManager] Error: Not initialized");
        return -1;
    }

    Serial.printf("[AnimationManager] Loading animation from memory (%zu bytes)\n", size);

    // Create sprite sheet
    SpriteSheet* sprite = new SpriteSheet();
    if (!sprite) {
        Serial.println("[AnimationManager] Error: Failed to create sprite");
        return -1;
    }

    // Load sprite from memory
    if (!sprite->loadFromMemory(data, size)) {
        Serial.printf("[AnimationManager] Error: Failed to load sprite: %s\n",
                     sprite->getErrorString());
        delete sprite;
        return -1;
    }

    // Check memory and create player (same as loadAnimation)
    size_t required = sprite->getMemoryUsed();
    if (!hasMemoryAvailable(required) && !evictLRU()) {
        delete sprite;
        return -1;
    }

    AnimationPlayer* player = nullptr;
    if (options.autoPlay) {
        player = new AnimationPlayer(sprite, parent);
        if (!player || player->getState() == AnimationState::ERROR) {
            delete player;
            delete sprite;
            return -1;
        }
        required += player->getStats().memoryUsed;
    }

    // Generate ID and create entry
    int32_t id = generateId();
    AnimationEntry entry;
    entry.sprite = sprite;
    entry.player = player;
    entry.parent = parent;  // Store parent for lazy player creation
    entry.lastAccessTime = millis();
    entry.useCount = 1;
    entry.isPinned = false;

    _animations[id] = entry;
    _memoryUsed += required;
    _totalLoads++;

    Serial.printf("[AnimationManager] ✓ Loaded animation #%d from memory\n", id);

    if (options.autoPlay && player) {
        player->play(options.loopMode);
    }

    return id;
}

void AnimationManager::unloadAnimation(int32_t animationId) {
    AnimationEntry* entry = getEntry(animationId);
    if (!entry) {
        return;
    }

    Serial.printf("[AnimationManager] Unloading animation #%d\n", animationId);

    size_t memoryFreed = 0;

    // Delete player and track its memory
    if (entry->player) {
        memoryFreed += entry->player->getStats().memoryUsed;
        delete entry->player;
        entry->player = nullptr;
    }

    // Delete sprite and track its memory
    if (entry->sprite) {
        memoryFreed += entry->sprite->getMemoryUsed();
        delete entry->sprite;
        entry->sprite = nullptr;
    }

    // Subtract total memory freed
    _memoryUsed -= memoryFreed;

    // Remove from map
    _animations.erase(animationId);

    Serial.printf("[AnimationManager] ✓ Unloaded (%zu KB freed, %zu KB remaining)\n",
                 memoryFreed / 1024, _memoryUsed / 1024);
}

void AnimationManager::unloadAll() {
    Serial.printf("[AnimationManager] Unloading all animations (%zu loaded)\n",
                 _animations.size());

    for (auto& pair : _animations) {
        AnimationEntry& entry = pair.second;

        if (entry.player) {
            delete entry.player;
        }
        if (entry.sprite) {
            delete entry.sprite;
        }
    }

    _animations.clear();
    _memoryUsed = 0;

    Serial.println("[AnimationManager] ✓ All animations unloaded");
}

// ==========================================
// Playback Control
// ==========================================

bool AnimationManager::playAnimation(int32_t animationId, LoopMode mode) {
    AnimationEntry* entry = getEntry(animationId);
    if (!entry) {
        Serial.printf("[AnimationManager] Error: Invalid animation ID %d\n", animationId);
        return false;
    }

    // Create player on-demand if it doesn't exist (lazy initialization)
    if (!entry->player) {
        Serial.printf("[AnimationManager] Creating player for animation #%d\n", animationId);

        if (!entry->parent) {
            Serial.println("[AnimationManager] Error: No parent screen stored");
            return false;
        }

        entry->player = new AnimationPlayer(entry->sprite, entry->parent);
        if (!entry->player || entry->player->getState() == AnimationState::ERROR) {
            Serial.println("[AnimationManager] Error: Failed to create player");
            delete entry->player;
            entry->player = nullptr;
            return false;
        }

        // Update memory usage
        _memoryUsed += entry->player->getStats().memoryUsed;
        Serial.printf("[AnimationManager] ✓ Player created (%zu KB total used)\n",
                     _memoryUsed / 1024);
    }

    entry->lastAccessTime = millis();
    entry->useCount++;

    Serial.printf("[AnimationManager] Playing animation #%d (mode=%d)\n",
                 animationId, (int)mode);
    return entry->player->play(mode);
}

void AnimationManager::pauseAnimation(int32_t animationId) {
    AnimationEntry* entry = getEntry(animationId);
    if (entry && entry->player) {
        entry->player->pause();
    }
}

void AnimationManager::resumeAnimation(int32_t animationId) {
    AnimationEntry* entry = getEntry(animationId);
    if (entry && entry->player) {
        entry->lastAccessTime = millis();
        entry->player->resume();
    }
}

void AnimationManager::stopAnimation(int32_t animationId) {
    AnimationEntry* entry = getEntry(animationId);
    if (entry && entry->player) {
        entry->player->stop();
    }
}

void AnimationManager::stopAll() {
    for (auto& pair : _animations) {
        if (pair.second.player) {
            pair.second.player->stop();
        }
    }
}

void AnimationManager::updateAll() {
    for (auto& pair : _animations) {
        if (pair.second.player) {
            pair.second.player->update();
        }
    }
}

// ==========================================
// Animation Control
// ==========================================

void AnimationManager::setSpeed(int32_t animationId, float speed) {
    AnimationEntry* entry = getEntry(animationId);
    if (entry && entry->player) {
        entry->player->setSpeed(speed);
    }
}

void AnimationManager::setPosition(int32_t animationId, int16_t x, int16_t y) {
    AnimationEntry* entry = getEntry(animationId);
    if (entry && entry->player) {
        entry->player->setPosition(x, y);
    }
}

void AnimationManager::setOpacity(int32_t animationId, uint8_t opacity) {
    AnimationEntry* entry = getEntry(animationId);
    if (entry && entry->player) {
        entry->player->setOpacity(opacity);
    }
}

void AnimationManager::setVisible(int32_t animationId, bool visible) {
    AnimationEntry* entry = getEntry(animationId);
    if (entry && entry->player) {
        entry->player->setVisible(visible);
    }
}

// ==========================================
// Status & Info
// ==========================================

bool AnimationManager::isValidAnimation(int32_t animationId) const {
    return _animations.find(animationId) != _animations.end();
}

AnimationPlayer* AnimationManager::getPlayer(int32_t animationId) {
    AnimationEntry* entry = getEntry(animationId);
    return entry ? entry->player : nullptr;
}

AnimationState AnimationManager::getState(int32_t animationId) const {
    const AnimationEntry* entry = getEntry(animationId);
    if (entry && entry->player) {
        return entry->player->getState();
    }
    return AnimationState::IDLE;
}

// ==========================================
// Cache Management
// ==========================================

void AnimationManager::pinAnimation(int32_t animationId) {
    AnimationEntry* entry = getEntry(animationId);
    if (entry) {
        entry->isPinned = true;
    }
}

void AnimationManager::unpinAnimation(int32_t animationId) {
    AnimationEntry* entry = getEntry(animationId);
    if (entry) {
        entry->isPinned = false;
    }
}

void AnimationManager::clearCache() {
    Serial.println("[AnimationManager] Clearing cache...");

    std::vector<int32_t> toRemove;

    for (auto& pair : _animations) {
        if (!pair.second.isPinned && !pair.second.player) {
            toRemove.push_back(pair.first);
        }
    }

    for (int32_t id : toRemove) {
        unloadAnimation(id);
    }

    Serial.printf("[AnimationManager] ✓ Cache cleared (%zu animations removed)\n",
                 toRemove.size());
}

// ==========================================
// Statistics
// ==========================================

AnimationManager::SystemStats AnimationManager::getSystemStats() const {
    SystemStats stats;
    stats.loadedCount = _animations.size();
    stats.playingCount = 0;
    stats.memoryUsed = _memoryUsed;
    stats.memoryAvailable = getMemoryAvailable();
    stats.totalLoads = _totalLoads;
    stats.cacheHits = _cacheHits;
    stats.cacheMisses = _cacheMisses;
    stats.evictions = _evictions;

    for (const auto& pair : _animations) {
        if (pair.second.player && pair.second.player->isPlaying()) {
            stats.playingCount++;
        }
    }

    return stats;
}

void AnimationManager::printSystemInfo() const {
    Serial.println("\n========== Animation System Info ==========");
    Serial.printf("Initialized: %s\n", _initialized ? "Yes" : "No");

    if (!_initialized) {
        Serial.println("===========================================\n");
        return;
    }

    SystemStats stats = getSystemStats();

    Serial.printf("Loaded Animations: %zu\n", stats.loadedCount);
    Serial.printf("Playing Animations: %zu\n", stats.playingCount);
    Serial.printf("Memory Used: %zu KB / %zu KB\n",
                 stats.memoryUsed / 1024, _cacheSizeLimit / 1024);
    Serial.printf("Memory Available: %zu KB\n", stats.memoryAvailable / 1024);
    Serial.printf("Total Loads: %u\n", stats.totalLoads);
    Serial.printf("Cache Hits: %u\n", stats.cacheHits);
    Serial.printf("Cache Misses: %u\n", stats.cacheMisses);
    Serial.printf("Evictions: %u\n", stats.evictions);
    Serial.println("===========================================\n");
}

// ==========================================
// Internal Methods
// ==========================================

int32_t AnimationManager::generateId() {
    return _nextId++;
}

AnimationEntry* AnimationManager::getEntry(int32_t animationId) {
    auto it = _animations.find(animationId);
    return (it != _animations.end()) ? &it->second : nullptr;
}

const AnimationEntry* AnimationManager::getEntry(int32_t animationId) const {
    auto it = _animations.find(animationId);
    return (it != _animations.end()) ? &it->second : nullptr;
}

bool AnimationManager::hasMemoryAvailable(size_t required) {
    return (getMemoryAvailable() >= required);
}

bool AnimationManager::evictLRU() {
    Serial.println("[AnimationManager] Attempting LRU eviction...");

    int32_t lruId = -1;
    uint32_t oldestTime = UINT32_MAX;

    // Find least recently used, unpinned animation
    for (const auto& pair : _animations) {
        if (!pair.second.isPinned &&
            pair.second.lastAccessTime < oldestTime) {
            oldestTime = pair.second.lastAccessTime;
            lruId = pair.first;
        }
    }

    if (lruId != -1) {
        Serial.printf("[AnimationManager] Evicting animation #%d (LRU)\n", lruId);
        unloadAnimation(lruId);
        _evictions++;
        return true;
    }

    Serial.println("[AnimationManager] No animations available for eviction");
    return false;
}

void AnimationManager::updateMemoryUsage() {
    _memoryUsed = 0;
    for (const auto& pair : _animations) {
        if (pair.second.sprite) {
            _memoryUsed += pair.second.sprite->getMemoryUsed();
        }
        if (pair.second.player) {
            _memoryUsed += pair.second.player->getStats().memoryUsed;
        }
    }
}

} // namespace Animation
} // namespace Doki
