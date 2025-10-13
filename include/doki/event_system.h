/**
 * @file event_system.h
 * @brief Event System for Doki OS - Pub/Sub messaging between components
 * 
 * The Event System allows components to communicate without tight coupling.
 * Components can publish events and subscribe to events they care about.
 * 
 * Example:
 *   // Subscribe to WiFi events
 *   EventSystem::subscribe(EVENT_WIFI_CONNECTED, onWiFiConnected);
 *   
 *   // Publish an event
 *   EventSystem::publish(EVENT_WIFI_CONNECTED);
 */

#ifndef DOKI_EVENT_SYSTEM_H
#define DOKI_EVENT_SYSTEM_H

#include <Arduino.h>
#include <functional>
#include <vector>

namespace Doki {

/**
 * @brief System-wide event types
 * 
 * Add new event types here as needed
 */
enum class EventType {
    // App lifecycle events
    APP_LOADED,           // App was loaded into memory
    APP_STARTED,          // App began running
    APP_PAUSED,           // App was paused
    APP_UNLOADED,         // App was unloaded from memory
    
    // Network events
    WIFI_CONNECTED,       // WiFi connection established
    WIFI_DISCONNECTED,    // WiFi connection lost
    WIFI_RECONNECTING,    // Attempting to reconnect
    
    // Time events
    TIME_SYNCED,          // NTP time synchronized
    TIME_UPDATED,         // Time was manually updated
    
    // System events
    LOW_MEMORY,           // Memory running low (<10%)
    SYSTEM_ERROR,         // System error occurred
    DISPLAY_READY,        // Display initialized
    
    // API events
    API_REQUEST_START,    // API request started
    API_REQUEST_SUCCESS,  // API request completed
    API_REQUEST_FAILED,   // API request failed
    
    // Custom events (for apps to use)
    CUSTOM_EVENT_1,
    CUSTOM_EVENT_2,
    CUSTOM_EVENT_3
};

/**
 * @brief Event data structure
 * 
 * Contains information about an event that was triggered
 */
struct Event {
    EventType type;           // Type of event
    const char* source;       // Component that published the event
    void* data;               // Optional event data (can be nullptr)
    uint32_t timestamp;       // When event was published (millis())
    
    Event(EventType t, const char* src, void* d = nullptr)
        : type(t), source(src), data(d), timestamp(millis()) {}
};

/**
 * @brief Event callback function type
 * 
 * Callback functions receive the event as parameter
 */
using EventCallback = std::function<void(const Event&)>;

/**
 * @brief Event System - Pub/Sub messaging for Doki OS
 * 
 * Singleton class that manages event publishing and subscription
 */
class EventSystem {
public:
    /**
     * @brief Subscribe to an event type
     * 
     * @param type Event type to listen for
     * @param callback Function to call when event occurs
     * @return Subscription ID (use to unsubscribe later)
     * 
     * Example:
     *   int id = EventSystem::subscribe(EVENT_WIFI_CONNECTED, [](const Event& e) {
     *       Serial.println("WiFi connected!");
     *   });
     */
    static int subscribe(EventType type, EventCallback callback);
    
    /**
     * @brief Unsubscribe from an event
     * 
     * @param subscriptionId ID returned from subscribe()
     * 
     * Example:
     *   EventSystem::unsubscribe(id);
     */
    static void unsubscribe(int subscriptionId);
    
    /**
     * @brief Publish an event
     * 
     * @param type Event type
     * @param source Component publishing the event (e.g., "WiFiManager")
     * @param data Optional event data (default: nullptr)
     * 
     * Example:
     *   EventSystem::publish(EVENT_WIFI_CONNECTED, "WiFiManager");
     *   EventSystem::publish(EVENT_APP_LOADED, "AppManager", (void*)appName);
     */
    static void publish(EventType type, const char* source, void* data = nullptr);
    
    /**
     * @brief Get number of subscribers for an event type
     * 
     * @param type Event type
     * @return Number of active subscribers
     */
    static int getSubscriberCount(EventType type);
    
    /**
     * @brief Clear all subscriptions (for testing/cleanup)
     */
    static void clearAll();
    
    /**
     * @brief Get event type name (for debugging)
     * 
     * @param type Event type
     * @return Human-readable event name
     */
    static const char* getEventName(EventType type);

private:
    /**
     * @brief Subscription entry
     */
    struct Subscription {
        int id;                   // Unique subscription ID
        EventType type;           // Event type this subscription listens for
        EventCallback callback;   // Function to call when event occurs
        bool active;              // Whether this subscription is active
        
        Subscription(int i, EventType t, EventCallback cb)
            : id(i), type(t), callback(cb), active(true) {}
    };
    
    // Static members
    static std::vector<Subscription> _subscriptions;  // All active subscriptions
    static int _nextSubscriptionId;                   // Counter for unique IDs
    
    // Helper methods
    static std::vector<Subscription*> _getSubscribers(EventType type);
};

} // namespace Doki

#endif // DOKI_EVENT_SYSTEM_H