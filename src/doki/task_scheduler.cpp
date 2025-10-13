/**
 * @file task_scheduler.cpp
 * @brief Implementation of Task Scheduler for Doki OS
 */

#include "doki/task_scheduler.h"
#include "doki/event_system.h"

namespace Doki {

// ========================================
// Static Member Initialization
// ========================================

std::map<int, TaskInfo> TaskScheduler::_tasks;
int TaskScheduler::_nextTaskId = 1;

// ========================================
// Public Methods
// ========================================

int TaskScheduler::createTask(const char* appId,
                              const char* taskName,
                              TaskFunction function,
                              size_t stackSize,
                              TaskPriority priority) {
    // Generate unique task ID
    int taskId = _nextTaskId++;
    
    // Create task info
    TaskInfo info;
    info.id = taskId;
    info.appId = appId;
    info.name = taskName;
    info.state = TaskState::CREATED;
    info.priority = priority;
    info.stackSize = stackSize;
    info.createdAt = millis();
    
    // Create task wrapper
    TaskWrapper* wrapper = new TaskWrapper(taskId, function);
    
    // Create FreeRTOS task
    BaseType_t result = xTaskCreate(
        _taskEntry,                          // Task entry point
        taskName,                            // Task name
        stackSize,                           // Stack size
        (void*)wrapper,                      // Parameter (our wrapper)
        _getPriority(priority),              // Priority
        &info.handle                         // Task handle
    );
    
    if (result != pdPASS) {
        Serial.printf("[TaskScheduler] Error: Failed to create task '%s' for app '%s'\n",
                      taskName, appId);
        delete wrapper;
        return -1;
    }
    
    // Store task info
    info.state = TaskState::RUNNING;
    _tasks[taskId] = info;
    
    Serial.printf("[TaskScheduler] Created task '%s' for app '%s' (ID: %d, Priority: %d)\n",
                  taskName, appId, taskId, (int)priority);
    
    return taskId;
}

bool TaskScheduler::stopTask(int taskId) {
    TaskInfo* info = _findTask(taskId);
    if (!info) {
        Serial.printf("[TaskScheduler] Task ID %d not found (may have already completed)\n", taskId);
        return false;
    }
    
    if (info->state == TaskState::STOPPED) {
        Serial.printf("[TaskScheduler] Warning: Task '%s' already stopped\n", 
                      info->name.c_str());
        return false;
    }
    
    // Only delete if handle is still valid
    if (info->handle != nullptr) {
        // Check if task still exists in FreeRTOS
        eTaskState taskState = eTaskGetState(info->handle);
        if (taskState != eDeleted && taskState != eInvalid) {
            vTaskDelete(info->handle);
            Serial.printf("[TaskScheduler] Stopped task '%s' (ID: %d)\n", 
                          info->name.c_str(), taskId);
        } else {
            Serial.printf("[TaskScheduler] Task '%s' (ID: %d) already completed\n", 
                          info->name.c_str(), taskId);
        }
        info->handle = nullptr;
    }
    
    info->state = TaskState::STOPPED;
    
    // Remove from map
    _tasks.erase(taskId);
    
    return true;
}

int TaskScheduler::stopAppTasks(const char* appId) {
    std::vector<int> tasksToStop;
    
    // Find all tasks for this app
    for (const auto& pair : _tasks) {
        if (pair.second.appId == appId) {
            tasksToStop.push_back(pair.first);
        }
    }
    
    // Stop each task
    int count = 0;
    for (int taskId : tasksToStop) {
        if (stopTask(taskId)) {
            count++;
        }
    }
    
    if (count > 0) {
        Serial.printf("[TaskScheduler] Stopped %d task(s) for app '%s'\n", count, appId);
    }
    
    return count;
}

bool TaskScheduler::suspendTask(int taskId) {
    TaskInfo* info = _findTask(taskId);
    if (!info) {
        return false;
    }
    
    if (info->state != TaskState::RUNNING) {
        Serial.printf("[TaskScheduler] Warning: Task '%s' not running\n", 
                      info->name.c_str());
        return false;
    }
    
    if (info->handle != nullptr) {
        vTaskSuspend(info->handle);
        info->state = TaskState::SUSPENDED;
        Serial.printf("[TaskScheduler] Suspended task '%s'\n", info->name.c_str());
        return true;
    }
    
    return false;
}

bool TaskScheduler::resumeTask(int taskId) {
    TaskInfo* info = _findTask(taskId);
    if (!info) {
        return false;
    }
    
    if (info->state != TaskState::SUSPENDED) {
        Serial.printf("[TaskScheduler] Warning: Task '%s' not suspended\n", 
                      info->name.c_str());
        return false;
    }
    
    if (info->handle != nullptr) {
        vTaskResume(info->handle);
        info->state = TaskState::RUNNING;
        Serial.printf("[TaskScheduler] Resumed task '%s'\n", info->name.c_str());
        return true;
    }
    
    return false;
}

TaskInfo TaskScheduler::getTaskInfo(int taskId) {
    TaskInfo* info = _findTask(taskId);
    if (info) {
        return *info;
    }
    return TaskInfo();  // Return empty if not found
}

std::vector<int> TaskScheduler::getAppTasks(const char* appId) {
    std::vector<int> result;
    
    for (const auto& pair : _tasks) {
        if (pair.second.appId == appId) {
            result.push_back(pair.first);
        }
    }
    
    return result;
}

int TaskScheduler::getActiveTaskCount() {
    int count = 0;
    for (const auto& pair : _tasks) {
        if (pair.second.state == TaskState::RUNNING || 
            pair.second.state == TaskState::SUSPENDED) {
            count++;
        }
    }
    return count;
}

void TaskScheduler::printTaskReport(const char* appId) {
    Serial.println("┌─────────────────────────────────────┐");
    if (appId) {
        Serial.printf("│ Task Report: %-22s │\n", appId);
    } else {
        Serial.println("│ Task Report: All Apps               │");
    }
    Serial.println("├─────────────────────────────────────┤");
    
    bool foundAny = false;
    for (const auto& pair : _tasks) {
        const TaskInfo& info = pair.second;
        
        // Filter by appId if specified
        if (appId && info.appId != appId) {
            continue;
        }
        
        foundAny = true;
        
        const char* stateStr = "UNKNOWN";
        switch (info.state) {
            case TaskState::CREATED:   stateStr = "CREATED"; break;
            case TaskState::RUNNING:   stateStr = "RUNNING"; break;
            case TaskState::SUSPENDED: stateStr = "SUSPENDED"; break;
            case TaskState::STOPPED:   stateStr = "STOPPED"; break;
        }
        
        Serial.printf("│ [%d] %s\n", info.id, info.name.c_str());
        Serial.printf("│   App: %s\n", info.appId.c_str());
        Serial.printf("│   State: %s\n", stateStr);
        Serial.printf("│   Priority: %d, Stack: %d bytes\n", 
                      (int)info.priority, info.stackSize);
        Serial.printf("│   Runtime: %lu ms\n", millis() - info.createdAt);
        Serial.println("│");
    }
    
    if (!foundAny) {
        Serial.println("│ No tasks found                      │");
    }
    
    Serial.println("└─────────────────────────────────────┘");
}

void TaskScheduler::stopAllTasks() {
    Serial.printf("[TaskScheduler] Stopping all tasks (%d total)\n", _tasks.size());
    
    std::vector<int> taskIds;
    for (const auto& pair : _tasks) {
        taskIds.push_back(pair.first);
    }
    
    for (int taskId : taskIds) {
        stopTask(taskId);
    }
}

TaskHandle_t TaskScheduler::getTaskHandle(int taskId) {
    TaskInfo* info = _findTask(taskId);
    return info ? info->handle : nullptr;
}

// ========================================
// Private Methods
// ========================================

void TaskScheduler::_taskEntry(void* parameter) {
    TaskWrapper* wrapper = static_cast<TaskWrapper*>(parameter);
    int taskId = wrapper ? wrapper->id : 0;
    
    if (wrapper && wrapper->function) {
        // Call the user's task function
        wrapper->function(wrapper->id);
    }
    
    // Task function returned, clean up wrapper
    delete wrapper;
    
    // Remove task from tracking map (before deleting self)
    if (taskId > 0) {
        auto it = _tasks.find(taskId);
        if (it != _tasks.end()) {
            Serial.printf("[TaskScheduler] Task '%s' (ID: %d) completed naturally\n", 
                          it->second.name.c_str(), taskId);
            _tasks.erase(it);
        }
    }
    
    // Delete self
    vTaskDelete(nullptr);
}

TaskInfo* TaskScheduler::_findTask(int taskId) {
    auto it = _tasks.find(taskId);
    if (it != _tasks.end()) {
        return &(it->second);
    }
    return nullptr;
}

UBaseType_t TaskScheduler::_getPriority(TaskPriority priority) {
    return static_cast<UBaseType_t>(priority);
}

} // namespace Doki