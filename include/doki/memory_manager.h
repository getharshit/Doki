/**
 * @file memory_manager.h
 * @brief Memory tracking and management for Doki OS
 * 
 * Tracks memory allocations per app to detect leaks and monitor usage.
 * Helps ensure apps clean up properly when unloaded.
 * 
 * Example:
 *   MemoryManager::startTracking("myapp");
 *   void* ptr = malloc(1024);
 *   MemoryManager::recordAllocation("myapp", 1024);
 *   // ... later ...
 *   free(ptr);
 *   MemoryManager::recordDeallocation("myapp", 1024);
 *   MemoryManager::stopTracking("myapp");  // Warns if memory not freed
 */

#ifndef DOKI_MEMORY_MANAGER_H
#define DOKI_MEMORY_MANAGER_H

#include <Arduino.h>
#include <map>
#include <string>

namespace Doki {

/**
 * @brief Memory statistics for an app
 */
struct MemoryStats {
    size_t heapAllocated;        // Bytes allocated in heap
    size_t psramAllocated;       // Bytes allocated in PSRAM
    size_t peakHeapUsage;        // Maximum heap used
    size_t peakPsramUsage;       // Maximum PSRAM used
    uint32_t allocationCount;    // Number of allocations
    uint32_t deallocationCount;  // Number of deallocations
    uint32_t trackingStartTime;  // When tracking started (millis())
    
    MemoryStats()
        : heapAllocated(0)
        , psramAllocated(0)
        , peakHeapUsage(0)
        , peakPsramUsage(0)
        , allocationCount(0)
        , deallocationCount(0)
        , trackingStartTime(0)
    {}
};

/**
 * @brief System memory information
 */
struct SystemMemory {
    size_t freeHeap;             // Free heap memory
    size_t totalHeap;            // Total heap memory
    size_t freePsram;            // Free PSRAM
    size_t totalPsram;           // Total PSRAM
    float heapUsagePercent;      // Heap usage percentage
    float psramUsagePercent;     // PSRAM usage percentage
};

/**
 * @brief Memory Manager - Tracks memory usage per app
 * 
 * Singleton class that monitors memory allocations and deallocations
 * to help detect memory leaks and optimize memory usage.
 */
class MemoryManager {
public:
    /**
     * @brief Start tracking memory for an app
     * 
     * @param appId App identifier
     * 
     * Call this when an app is created. All subsequent allocations
     * should be recorded until stopTracking() is called.
     * 
     * Example:
     *   MemoryManager::startTracking("clock");
     */
    static void startTracking(const char* appId);
    
    /**
     * @brief Stop tracking memory for an app
     * 
     * @param appId App identifier
     * @return true if app freed all memory, false if leaks detected
     * 
     * Call this when an app is destroyed. Checks if all allocated
     * memory was properly freed.
     * 
     * Example:
     *   bool clean = MemoryManager::stopTracking("clock");
     *   if (!clean) {
     *       Serial.println("Memory leak detected!");
     *   }
     */
    static bool stopTracking(const char* appId);
    
    /**
     * @brief Record a memory allocation
     * 
     * @param appId App identifier
     * @param bytes Number of bytes allocated
     * @param isPsram Whether allocation is in PSRAM (default: false)
     * 
     * Call this whenever an app allocates memory.
     * 
     * Example:
     *   void* buf = malloc(1024);
     *   MemoryManager::recordAllocation("clock", 1024, false);
     */
    static void recordAllocation(const char* appId, size_t bytes, bool isPsram = false);
    
    /**
     * @brief Record a memory deallocation
     * 
     * @param appId App identifier
     * @param bytes Number of bytes freed
     * @param isPsram Whether deallocation is in PSRAM (default: false)
     * 
     * Call this whenever an app frees memory.
     * 
     * Example:
     *   free(buf);
     *   MemoryManager::recordDeallocation("clock", 1024, false);
     */
    static void recordDeallocation(const char* appId, size_t bytes, bool isPsram = false);
    
    /**
     * @brief Get memory statistics for an app
     * 
     * @param appId App identifier
     * @return Memory statistics (empty if app not tracked)
     */
    static MemoryStats getAppStats(const char* appId);
    
    /**
     * @brief Get current system memory information
     * 
     * @return System memory stats
     */
    static SystemMemory getSystemMemory();
    
    /**
     * @brief Print memory report for an app
     * 
     * @param appId App identifier
     * 
     * Prints detailed memory usage to Serial.
     * 
     * Example output:
     *   [MemoryManager] App: clock
     *     Heap: 2048 bytes (Peak: 2048)
     *     PSRAM: 0 bytes (Peak: 0)
     *     Allocations: 1, Deallocations: 0
     */
    static void printAppReport(const char* appId);
    
    /**
     * @brief Print system memory report
     * 
     * Prints current system memory status to Serial.
     * 
     * Example output:
     *   [MemoryManager] System Memory:
     *     Heap: 304720 / 327680 bytes (7% used)
     *     PSRAM: 2056671 / 2097152 bytes (2% used)
     */
    static void printSystemReport();
    
    /**
     * @brief Check if system memory is low
     * 
     * @param threshold Warning threshold (0.0 to 1.0, default: 0.9)
     * @return true if either heap or PSRAM usage exceeds threshold
     * 
     * Example:
     *   if (MemoryManager::isLowMemory(0.9)) {
     *       Serial.println("Warning: Memory usage above 90%!");
     *   }
     */
    static bool isLowMemory(float threshold = 0.9f);
    
    /**
     * @brief Clear all tracking data
     * 
     * Removes all app memory records. Useful for testing/reset.
     */
    static void clearAll();
    
    /**
     * @brief Get number of apps currently being tracked
     * 
     * @return Number of apps with active memory tracking
     */
    static int getTrackedAppCount();

private:
    // Map of app IDs to their memory statistics
    static std::map<std::string, MemoryStats> _appStats;
    
    // Low memory warning threshold (published as event)
    static const float LOW_MEMORY_THRESHOLD;
    
    // Helper: Get or create stats for an app
    static MemoryStats* _getOrCreateStats(const char* appId);
    
    // Helper: Check and warn about low memory
    static void _checkLowMemory();
};

} // namespace Doki

#endif // DOKI_MEMORY_MANAGER_H