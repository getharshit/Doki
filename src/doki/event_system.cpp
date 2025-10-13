/**
 * @file event_system.cpp
 * @brief Implementation of Event System for Doki OS
 */

#include "doki/event_system.h"

namespace Doki {

// ========================================
// Static Member Initialization
// ========================================

std::vector<EventSystem::Subscription> EventSystem::_subscriptions;
int EventSystem::_nextSubscriptionId = 1;

// ========================================
// Public Methods
// ========================================

int EventSystem::subscribe(EventType type, EventCallback callback) {
    // Generate unique subscription ID
    int id = _nextSubscriptionId++;
    
    // Add subscription to list
    _subscriptions.emplace_back(id, type, callback);
    
    Serial.printf("[EventSystem] Subscribed to %s (ID: %d, Total subscribers: %d)\n",
                  getEventName(type), id, getSubscriberCount(type));
    
    return id;
}

void EventSystem::unsubscribe(int subscriptionId) {
    // Find and deactivate the subscription
    for (auto& sub : _subscriptions) {
        if (sub.id == subscriptionId && sub.active) {
            sub.active = false;
            Serial.printf("[EventSystem] Unsubscribed ID %d from %s\n",
                          subscriptionId, getEventName(sub.type));
            return;
        }
    }
    
    Serial.printf("[EventSystem] Warning: Subscription ID %d not found\n", subscriptionId);
}

void EventSystem::publish(EventType type, const char* source, void* data) {
    // Create event
    Event event(type, source, data);
    
    // Get all subscribers for this event type
    std::vector<Subscription*> subscribers = _getSubscribers(type);
    
    Serial.printf("[EventSystem] Publishing %s from '%s' (%d subscribers)\n",
                  getEventName(type), source, subscribers.size());
    
    // Call all subscriber callbacks
    for (auto* sub : subscribers) {
        if (sub->active) {
            try {
                sub->callback(event);
            } catch (const std::exception& e) {
                Serial.printf("[EventSystem] Error in subscriber callback: %s\n", e.what());
            } catch (...) {
                Serial.println("[EventSystem] Unknown error in subscriber callback");
            }
        }
    }
}

int EventSystem::getSubscriberCount(EventType type) {
    int count = 0;
    for (const auto& sub : _subscriptions) {
        if (sub.type == type && sub.active) {
            count++;
        }
    }
    return count;
}

void EventSystem::clearAll() {
    Serial.printf("[EventSystem] Clearing all subscriptions (%d total)\n", 
                  _subscriptions.size());
    _subscriptions.clear();
    _nextSubscriptionId = 1;
}

const char* EventSystem::getEventName(EventType type) {
    switch (type) {
        // App lifecycle events
        case EventType::APP_LOADED:         return "APP_LOADED";
        case EventType::APP_STARTED:        return "APP_STARTED";
        case EventType::APP_PAUSED:         return "APP_PAUSED";
        case EventType::APP_UNLOADED:       return "APP_UNLOADED";
        
        // Network events
        case EventType::WIFI_CONNECTED:     return "WIFI_CONNECTED";
        case EventType::WIFI_DISCONNECTED:  return "WIFI_DISCONNECTED";
        case EventType::WIFI_RECONNECTING:  return "WIFI_RECONNECTING";
        
        // Time events
        case EventType::TIME_SYNCED:        return "TIME_SYNCED";
        case EventType::TIME_UPDATED:       return "TIME_UPDATED";
        
        // System events
        case EventType::LOW_MEMORY:         return "LOW_MEMORY";
        case EventType::SYSTEM_ERROR:       return "SYSTEM_ERROR";
        case EventType::DISPLAY_READY:      return "DISPLAY_READY";
        
        // API events
        case EventType::API_REQUEST_START:   return "API_REQUEST_START";
        case EventType::API_REQUEST_SUCCESS: return "API_REQUEST_SUCCESS";
        case EventType::API_REQUEST_FAILED:  return "API_REQUEST_FAILED";
        
        // Custom events
        case EventType::CUSTOM_EVENT_1:     return "CUSTOM_EVENT_1";
        case EventType::CUSTOM_EVENT_2:     return "CUSTOM_EVENT_2";
        case EventType::CUSTOM_EVENT_3:     return "CUSTOM_EVENT_3";
        
        default:                            return "UNKNOWN_EVENT";
    }
}

// ========================================
// Private Helper Methods
// ========================================

std::vector<EventSystem::Subscription*> EventSystem::_getSubscribers(EventType type) {
    std::vector<Subscription*> result;
    
    for (auto& sub : _subscriptions) {
        if (sub.type == type && sub.active) {
            result.push_back(&sub);
        }
    }
    
    return result;
}

} // namespace Doki