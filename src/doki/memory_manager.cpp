/**
 * @file memory_manager.cpp
 * @brief Implementation of Memory Manager for Doki OS
 */

#include "doki/memory_manager.h"
#include "doki/event_system.h"

namespace Doki {

// ========================================
// Static Member Initialization
// ========================================

std::map<std::string, MemoryStats> MemoryManager::_appStats;
const float MemoryManager::LOW_MEMORY_THRESHOLD = 0.9f;  // 90%

// ========================================
// Public Methods
// ========================================

void MemoryManager::startTracking(const char* appId) {
    MemoryStats* stats = _getOrCreateStats(appId);
    stats->trackingStartTime = millis();
    
    Serial.printf("[MemoryManager] Started tracking: %s\n", appId);
}

bool MemoryManager::stopTracking(const char* appId) {
    auto it = _appStats.find(appId);
    if (it == _appStats.end()) {
        Serial.printf("[MemoryManager] Warning: App '%s' not being tracked\n", appId);
        return true;
    }
    
    MemoryStats& stats = it->second;
    
    // Check for memory leaks
    bool hasLeaks = (stats.heapAllocated > 0 || stats.psramAllocated > 0);
    
    if (hasLeaks) {
        Serial.printf("[MemoryManager] ⚠️  MEMORY LEAK DETECTED in '%s'!\n", appId);
        Serial.printf("  Heap not freed: %d bytes\n", stats.heapAllocated);
        Serial.printf("  PSRAM not freed: %d bytes\n", stats.psramAllocated);
        
        // Publish low memory event
        EventSystem::publish(EventType::SYSTEM_ERROR, "MemoryManager", (void*)appId);
    } else {
        Serial.printf("[MemoryManager] ✓ Clean shutdown: %s (no leaks)\n", appId);
    }
    
    // Print final report
    printAppReport(appId);
    
    // Remove from tracking
    _appStats.erase(it);
    
    return !hasLeaks;
}

void MemoryManager::recordAllocation(const char* appId, size_t bytes, bool isPsram) {
    MemoryStats* stats = _getOrCreateStats(appId);
    
    if (isPsram) {
        stats->psramAllocated += bytes;
        if (stats->psramAllocated > stats->peakPsramUsage) {
            stats->peakPsramUsage = stats->psramAllocated;
        }
    } else {
        stats->heapAllocated += bytes;
        if (stats->heapAllocated > stats->peakHeapUsage) {
            stats->peakHeapUsage = stats->heapAllocated;
        }
    }
    
    stats->allocationCount++;
    
    // Check for low memory condition
    _checkLowMemory();
}

void MemoryManager::recordDeallocation(const char* appId, size_t bytes, bool isPsram) {
    MemoryStats* stats = _getOrCreateStats(appId);
    
    if (isPsram) {
        if (stats->psramAllocated >= bytes) {
            stats->psramAllocated -= bytes;
        } else {
            Serial.printf("[MemoryManager] Warning: %s trying to free more PSRAM than allocated!\n", appId);
            stats->psramAllocated = 0;
        }
    } else {
        if (stats->heapAllocated >= bytes) {
            stats->heapAllocated -= bytes;
        } else {
            Serial.printf("[MemoryManager] Warning: %s trying to free more heap than allocated!\n", appId);
            stats->heapAllocated = 0;
        }
    }
    
    stats->deallocationCount++;
}

MemoryStats MemoryManager::getAppStats(const char* appId) {
    auto it = _appStats.find(appId);
    if (it != _appStats.end()) {
        return it->second;
    }
    return MemoryStats();  // Return empty stats if not found
}

SystemMemory MemoryManager::getSystemMemory() {
    SystemMemory mem;
    
    // Get heap information
    mem.freeHeap = ESP.getFreeHeap();
    mem.totalHeap = ESP.getHeapSize();
    
    // Get PSRAM information
    mem.freePsram = ESP.getFreePsram();
    mem.totalPsram = ESP.getPsramSize();
    
    // Calculate usage percentages
    if (mem.totalHeap > 0) {
        mem.heapUsagePercent = (float)(mem.totalHeap - mem.freeHeap) / mem.totalHeap;
    } else {
        mem.heapUsagePercent = 0.0f;
    }
    
    if (mem.totalPsram > 0) {
        mem.psramUsagePercent = (float)(mem.totalPsram - mem.freePsram) / mem.totalPsram;
    } else {
        mem.psramUsagePercent = 0.0f;
    }
    
    return mem;
}

void MemoryManager::printAppReport(const char* appId) {
    auto it = _appStats.find(appId);
    if (it == _appStats.end()) {
        Serial.printf("[MemoryManager] No tracking data for '%s'\n", appId);
        return;
    }
    
    const MemoryStats& stats = it->second;
    uint32_t runtime = millis() - stats.trackingStartTime;
    
    Serial.println("┌─────────────────────────────────────┐");
    Serial.printf("│ Memory Report: %-20s │\n", appId);
    Serial.println("├─────────────────────────────────────┤");
    Serial.printf("│ Runtime: %lu ms                     \n", runtime);
    Serial.printf("│ Heap Allocated: %d bytes            \n", stats.heapAllocated);
    Serial.printf("│ Heap Peak: %d bytes                 \n", stats.peakHeapUsage);
    Serial.printf("│ PSRAM Allocated: %d bytes           \n", stats.psramAllocated);
    Serial.printf("│ PSRAM Peak: %d bytes                \n", stats.peakPsramUsage);
    Serial.printf("│ Allocations: %lu                    \n", stats.allocationCount);
    Serial.printf("│ Deallocations: %lu                  \n", stats.deallocationCount);
    Serial.println("└─────────────────────────────────────┘");
}

void MemoryManager::printSystemReport() {
    SystemMemory mem = getSystemMemory();
    
    Serial.println("┌─────────────────────────────────────┐");
    Serial.println("│ System Memory Report                │");
    Serial.println("├─────────────────────────────────────┤");
    Serial.printf("│ Heap:  %d / %d bytes (%.1f%% used) \n", 
                  mem.totalHeap - mem.freeHeap, 
                  mem.totalHeap, 
                  mem.heapUsagePercent * 100.0f);
    Serial.printf("│ PSRAM: %d / %d bytes (%.1f%% used) \n", 
                  mem.totalPsram - mem.freePsram, 
                  mem.totalPsram, 
                  mem.psramUsagePercent * 100.0f);
    Serial.println("└─────────────────────────────────────┘");
}

bool MemoryManager::isLowMemory(float threshold) {
    SystemMemory mem = getSystemMemory();
    return (mem.heapUsagePercent >= threshold || mem.psramUsagePercent >= threshold);
}

void MemoryManager::clearAll() {
    Serial.printf("[MemoryManager] Clearing all tracking data (%d apps)\n", _appStats.size());
    _appStats.clear();
}

int MemoryManager::getTrackedAppCount() {
    return _appStats.size();
}

// ========================================
// Private Helper Methods
// ========================================

MemoryStats* MemoryManager::_getOrCreateStats(const char* appId) {
    auto it = _appStats.find(appId);
    if (it == _appStats.end()) {
        // Create new stats entry
        _appStats[appId] = MemoryStats();
        it = _appStats.find(appId);
    }
    return &(it->second);
}

void MemoryManager::_checkLowMemory() {
    static uint32_t lastCheck = 0;
    uint32_t now = millis();
    
    // Check every 5 seconds
    if (now - lastCheck < 5000) {
        return;
    }
    lastCheck = now;
    
    if (isLowMemory(LOW_MEMORY_THRESHOLD)) {
        SystemMemory mem = getSystemMemory();
        Serial.printf("[MemoryManager] ⚠️  LOW MEMORY WARNING!\n");
        Serial.printf("  Heap: %.1f%% used\n", mem.heapUsagePercent * 100.0f);
        Serial.printf("  PSRAM: %.1f%% used\n", mem.psramUsagePercent * 100.0f);
        
        // Publish low memory event
        EventSystem::publish(EventType::LOW_MEMORY, "MemoryManager");
    }
}

} // namespace Doki