/**
 * @file task_scheduler.h
 * @brief Task Scheduler for Doki OS - FreeRTOS wrapper for easy task management
 * 
 * Simplifies FreeRTOS task creation and management for apps.
 * Automatically cleans up tasks when apps are unloaded.
 * 
 * Example:
 *   // Create a background task
 *   int taskId = TaskScheduler::createTask("myapp", "UpdateTask", 
 *                                          myTaskFunction, 2048, 5);
 *   
 *   // Later, stop the task
 *   TaskScheduler::stopTask(taskId);
 *   
 *   // Or stop all tasks for an app
 *   TaskScheduler::stopAppTasks("myapp");
 */

#ifndef DOKI_TASK_SCHEDULER_H
#define DOKI_TASK_SCHEDULER_H

#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <map>
#include <vector>
#include <string>
#include <functional>

namespace Doki {

/**
 * @brief Task function type
 * 
 * Tasks receive their task ID as parameter
 */
using TaskFunction = std::function<void(int taskId)>;

/**
 * @brief Task priority levels (matches FreeRTOS priorities)
 */
enum class TaskPriority {
    BACKGROUND = 0,     // Lowest priority (background tasks)
    LOWER = 1,          // Low priority
    NORMAL = 5,         // Normal priority (default)
    HIGHER = 10,        // High priority
    CRITICAL = 15       // Highest priority (time-critical tasks)
};

/**
 * @brief Task state
 */
enum class TaskState {
    CREATED,      // Task created but not started
    RUNNING,      // Task is running
    SUSPENDED,    // Task is suspended
    STOPPED       // Task has been stopped
};

/**
 * @brief Task information
 */
struct TaskInfo {
    int id;                      // Unique task ID
    std::string appId;           // App that owns this task
    std::string name;            // Task name
    TaskHandle_t handle;         // FreeRTOS task handle
    TaskState state;             // Current state
    TaskPriority priority;       // Task priority
    size_t stackSize;            // Stack size in bytes
    uint32_t createdAt;          // Creation timestamp
    
    TaskInfo()
        : id(0), handle(nullptr), state(TaskState::CREATED)
        , priority(TaskPriority::NORMAL), stackSize(0), createdAt(0) {}
};

/**
 * @brief Task Scheduler - Manages FreeRTOS tasks for Doki OS
 * 
 * Singleton class that simplifies task creation and management
 */
class TaskScheduler {
public:
    /**
     * @brief Create a new task
     * 
     * @param appId App identifier (for cleanup)
     * @param taskName Task name (for debugging)
     * @param function Task function to execute
     * @param stackSize Stack size in bytes (default: 4096)
     * @param priority Task priority (default: NORMAL)
     * @return Task ID (>0 on success, -1 on failure)
     * 
     * Example:
     *   int taskId = TaskScheduler::createTask("clock", "TimeUpdate",
     *       [](int id) {
     *           while(true) {
     *               Serial.println("Updating time...");
     *               vTaskDelay(pdMS_TO_TICKS(1000));
     *           }
     *       }, 4096, TaskPriority::NORMAL);
     */
    static int createTask(const char* appId, 
                         const char* taskName,
                         TaskFunction function,
                         size_t stackSize = 4096,
                         TaskPriority priority = TaskPriority::NORMAL);
    
    /**
     * @brief Stop a task
     * 
     * @param taskId Task ID returned from createTask()
     * @return true if task stopped successfully
     * 
     * Safely stops and deletes the task.
     */
    static bool stopTask(int taskId);
    
    /**
     * @brief Stop all tasks for an app
     * 
     * @param appId App identifier
     * @return Number of tasks stopped
     * 
     * Called automatically when an app is unloaded.
     */
    static int stopAppTasks(const char* appId);
    
    /**
     * @brief Suspend a task (pause execution)
     * 
     * @param taskId Task ID
     * @return true if task suspended successfully
     */
    static bool suspendTask(int taskId);
    
    /**
     * @brief Resume a suspended task
     * 
     * @param taskId Task ID
     * @return true if task resumed successfully
     */
    static bool resumeTask(int taskId);
    
    /**
     * @brief Get task information
     * 
     * @param taskId Task ID
     * @return Task information (empty if not found)
     */
    static TaskInfo getTaskInfo(int taskId);
    
    /**
     * @brief Get all tasks for an app
     * 
     * @param appId App identifier
     * @return Vector of task IDs
     */
    static std::vector<int> getAppTasks(const char* appId);
    
    /**
     * @brief Get total number of active tasks
     * 
     * @return Number of tasks currently running
     */
    static int getActiveTaskCount();
    
    /**
     * @brief Print task report
     * 
     * @param appId Optional: App ID to filter by (nullptr = all tasks)
     * 
     * Prints detailed task information to Serial.
     */
    static void printTaskReport(const char* appId = nullptr);
    
    /**
     * @brief Stop all tasks (cleanup)
     * 
     * Stops and deletes all managed tasks.
     * Use for system shutdown or testing.
     */
    static void stopAllTasks();
    
    /**
     * @brief Get FreeRTOS task handle
     * 
     * @param taskId Task ID
     * @return FreeRTOS task handle (nullptr if not found)
     * 
     * For advanced FreeRTOS operations.
     */
    static TaskHandle_t getTaskHandle(int taskId);

private:
    // Internal task wrapper structure
    struct TaskWrapper {
        int id;
        TaskFunction function;
        bool shouldStop;
        
        TaskWrapper(int i, TaskFunction f) 
            : id(i), function(f), shouldStop(false) {}
    };
    
    // Static members
    static std::map<int, TaskInfo> _tasks;           // All managed tasks
    static int _nextTaskId;                          // Counter for unique IDs
    
    // FreeRTOS task entry point (static wrapper)
    static void _taskEntry(void* parameter);
    
    // Helper: Find task by ID
    static TaskInfo* _findTask(int taskId);
    
    // Helper: Convert priority enum to FreeRTOS priority
    static UBaseType_t _getPriority(TaskPriority priority);
};

} // namespace Doki

#endif // DOKI_TASK_SCHEDULER_H