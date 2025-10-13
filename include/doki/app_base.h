/**
 * @file app_base.h
 * @brief Base class for all Doki OS applications
 * 
 * All apps in Doki OS must inherit from this class and implement
 * the lifecycle methods. This ensures consistent behavior across
 * all applications.
 * 
 * Lifecycle:
 * 1. onCreate()  - Initialize app, create UI elements
 * 2. onStart()   - App becomes visible on display
 * 3. onUpdate()  - Called every frame (app can skip if needed)
 * 4. onPause()   - App goes to background (optional)
 * 5. onDestroy() - Cleanup before unload
 */

#ifndef DOKI_APP_BASE_H
#define DOKI_APP_BASE_H

#include <Arduino.h>
#include <lvgl.h>

namespace Doki {

/**
 * @brief App lifecycle states
 */
enum class AppState {
    IDLE,        // App not loaded
    CREATED,     // onCreate() called
    STARTED,     // onStart() called, app is visible
    PAUSED,      // onPause() called, app in background
    DESTROYED    // onDestroy() called
};

/**
 * @brief Base class for all Doki OS applications
 * 
 * Example usage:
 * 
 * class MyApp : public DokiApp {
 * public:
 *     MyApp() : DokiApp("myapp", "My App") {}
 *     
 *     void onCreate() override {
 *         // Create UI elements
 *         label = lv_label_create(lv_scr_act());
 *         lv_label_set_text(label, "Hello!");
 *     }
 *     
 *     void onDestroy() override {
 *         // LVGL auto-cleans children, but clean up other resources
 *     }
 * };
 */
class DokiApp {
public:
    /**
     * @brief Constructor
     * @param id Unique app identifier (e.g., "clock", "weather")
     * @param name Human-readable app name (e.g., "Clock", "Weather")
     */
    DokiApp(const char* id, const char* name);
    
    /**
     * @brief Virtual destructor
     */
    virtual ~DokiApp();
    
    // ========================================
    // Lifecycle Methods (override these!)
    // ========================================
    
    /**
     * @brief Initialize app - create UI elements, load resources
     * 
     * Called once when app is first loaded. Use this to:
     * - Create LVGL UI elements
     * - Load configuration
     * - Initialize variables
     * - Allocate memory
     * 
     * Note: Display is available, but app is not visible yet
     */
    virtual void onCreate() = 0;
    
    /**
     * @brief App becomes visible on display
     * 
     * Called when app should start displaying content.
     * Use this to:
     * - Start animations
     * - Begin data updates
     * - Subscribe to events
     */
    virtual void onStart() = 0;
    
    /**
     * @brief Update app state (called every frame)
     * 
     * Called repeatedly while app is running (typically 30-60 FPS).
     * Apps can check elapsed time to decide if update is needed.
     * 
     * Example:
     * void onUpdate() {
     *     uint32_t now = millis();
     *     if (now - lastUpdate >= 1000) {  // Update every 1 second
     *         // Update UI
     *         lastUpdate = now;
     *     }
     * }
     */
    virtual void onUpdate() = 0;
    
    /**
     * @brief App goes to background (optional)
     * 
     * Called when another app is about to load.
     * Use this to:
     * - Pause animations
     * - Stop timers
     * - Save state
     * 
     * Note: Not always called (depends on app manager implementation)
     */
    virtual void onPause() {}
    
    /**
     * @brief Cleanup before app unload
     * 
     * Called before app is unloaded. Use this to:
     * - Free allocated memory
     * - Close files/connections
     * - Unsubscribe from events
     * - Save persistent data
     * 
     * Note: LVGL automatically cleans up UI elements created on screen
     */
    virtual void onDestroy() = 0;
    
    // ========================================
    // App Information (getters)
    // ========================================
    
    /**
     * @brief Get app unique ID
     * @return App ID string (e.g., "clock")
     */
    const char* getId() const { return _id; }
    
    /**
     * @brief Get app display name
     * @return App name string (e.g., "Clock")
     */
    const char* getName() const { return _name; }
    
    /**
     * @brief Get current app state
     * @return Current lifecycle state
     */
    AppState getState() const { return _state; }
    
    /**
     * @brief Get app uptime in milliseconds
     * @return Time since app was started (onStart called)
     */
    uint32_t getUptime() const;
    
    /**
     * @brief Check if app is currently running
     * @return true if app is in STARTED state
     */
    bool isRunning() const { return _state == AppState::STARTED; }

protected:
    // ========================================
    // Helper Methods (for subclasses)
    // ========================================
    
    /**
     * @brief Clear the screen (remove all LVGL objects)
     * 
     * Useful for cleanup. LVGL will automatically delete
     * all children when screen is cleaned.
     */
    void clearScreen();
    
    /**
     * @brief Get the LVGL screen object
     * @return Pointer to active screen
     */
    lv_obj_t* getScreen() { return lv_scr_act(); }
    
    /**
     * @brief Log message from app
     * @param message Message to log
     */
    void log(const char* message);
    
public:
    // TEMPORARY: For testing without AppManager
    // These will be made private once AppManager is implemented
    void _setState(AppState state);
    void _markStarted();

private:
    // App identification
    const char* _id;              // Unique ID (e.g., "clock")
    const char* _name;            // Display name (e.g., "Clock")
    
    // Lifecycle tracking
    AppState _state;              // Current state
    uint32_t _startTime;          // When onStart() was called
    
    // Internal lifecycle management (called by AppManager)
    friend class AppManager;
};

} // namespace Doki

#endif // DOKI_APP_BASE_H