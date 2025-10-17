/**
 * @file animation_player.h
 * @brief Animation playback controller for Doki OS
 *
 * Controls playback of loaded sprite sheets with support for
 * play/pause/stop, looping, speed control, and frame callbacks.
 */

#ifndef DOKI_ANIMATION_ANIMATION_PLAYER_H
#define DOKI_ANIMATION_ANIMATION_PLAYER_H

#include <Arduino.h>
#include <lvgl.h>
#include "animation_types.h"
#include "sprite_sheet.h"

namespace Doki {
namespace Animation {

/**
 * AnimationPlayer class
 *
 * Controls playback of a sprite sheet animation.
 * Handles frame timing, looping, and LVGL canvas updates.
 *
 * Usage:
 *   AnimationPlayer* player = new AnimationPlayer(sprite, screen);
 *   player->setPosition(120, 160);
 *   player->play(LoopMode::LOOP);
 */
class AnimationPlayer {
public:
    // ==========================================
    // Constructor / Destructor
    // ==========================================

    /**
     * Create animation player
     * @param sprite Sprite sheet to play (must remain valid)
     * @param parent LVGL parent object (usually screen)
     */
    AnimationPlayer(SpriteSheet* sprite, lv_obj_t* parent);

    ~AnimationPlayer();

    // Prevent copying
    AnimationPlayer(const AnimationPlayer&) = delete;
    AnimationPlayer& operator=(const AnimationPlayer&) = delete;

    // ==========================================
    // Playback Control
    // ==========================================

    /**
     * Start playback
     * @param mode Loop mode (ONCE, LOOP, PING_PONG)
     * @return true if started successfully
     */
    bool play(LoopMode mode = LoopMode::ONCE);

    /**
     * Pause playback
     */
    void pause();

    /**
     * Resume playback
     */
    void resume();

    /**
     * Stop playback and reset to first frame
     */
    void stop();

    /**
     * Update animation (call every frame in main loop)
     * @return true if frame was updated
     */
    bool update();

    // ==========================================
    // Playback Settings
    // ==========================================

    /**
     * Set playback speed multiplier
     * @param speed Speed multiplier (0.1 - 10.0)
     *              1.0 = normal, 2.0 = double speed, 0.5 = half speed
     */
    void setSpeed(float speed);

    /**
     * Get current speed multiplier
     */
    float getSpeed() const { return _speed; }

    /**
     * Set loop mode (can change during playback)
     */
    void setLoopMode(LoopMode mode) { _loopMode = mode; }

    /**
     * Get current loop mode
     */
    LoopMode getLoopMode() const { return _loopMode; }

    // ==========================================
    // Position & Transform
    // ==========================================

    /**
     * Set animation position
     * @param x X coordinate
     * @param y Y coordinate
     */
    void setPosition(int16_t x, int16_t y);

    /**
     * Get current position
     */
    void getPosition(int16_t& x, int16_t& y) const {
        x = _transform.x;
        y = _transform.y;
    }

    /**
     * Set opacity (0-255)
     */
    void setOpacity(uint8_t opacity);

    /**
     * Get opacity
     */
    uint8_t getOpacity() const { return _transform.opacity; }

    /**
     * Show/hide animation
     */
    void setVisible(bool visible);

    /**
     * Check if visible
     */
    bool isVisible() const;

    // ==========================================
    // Frame Control
    // ==========================================

    /**
     * Jump to specific frame
     * @param frameIndex Frame index (0 to frameCount-1)
     */
    void gotoFrame(uint16_t frameIndex);

    /**
     * Go to next frame
     */
    void nextFrame();

    /**
     * Go to previous frame
     */
    void prevFrame();

    /**
     * Get current frame index
     */
    uint16_t getCurrentFrame() const { return _currentFrame; }

    /**
     * Get total frame count
     */
    uint16_t getFrameCount() const {
        return _sprite ? _sprite->getFrameCount() : 0;
    }

    // ==========================================
    // Status
    // ==========================================

    /**
     * Get current playback state
     */
    AnimationState getState() const { return _state; }

    /**
     * Check if playing
     */
    bool isPlaying() const { return _state == AnimationState::PLAYING; }

    /**
     * Check if paused
     */
    bool isPaused() const { return _state == AnimationState::PAUSED; }

    /**
     * Get playback statistics
     */
    const AnimationStats& getStats() const { return _stats; }

    /**
     * Reset statistics
     */
    void resetStats();

    // ==========================================
    // LVGL Objects
    // ==========================================

    /**
     * Get LVGL canvas object
     */
    lv_obj_t* getCanvas() { return _canvas; }

    /**
     * Get LVGL image descriptor
     */
    const lv_img_dsc_t* getImageDescriptor() const { return &_imgDsc; }

private:
    // ==========================================
    // Internal Methods
    // ==========================================

    /**
     * Create LVGL canvas and image
     */
    bool createCanvas();

    /**
     * Update canvas with current frame
     */
    void updateCanvas();

    /**
     * Convert 8-bit indexed color to RGB565
     */
    void convertFrameToRGB565(const uint8_t* frameData, uint16_t* output);

    /**
     * Calculate next frame based on loop mode
     */
    uint16_t calculateNextFrame();

    /**
     * Check if should advance frame based on timing
     */
    bool shouldAdvanceFrame();

    // ==========================================
    // Member Variables
    // ==========================================

    // Sprite data
    SpriteSheet* _sprite;               // Sprite sheet reference
    lv_obj_t* _parent;                  // LVGL parent object

    // LVGL objects
    lv_obj_t* _canvas;                  // LVGL canvas for rendering
    lv_img_dsc_t _imgDsc;               // Image descriptor
    uint16_t* _canvasBuffer;            // Canvas buffer (RGB565, in PSRAM)
    size_t _canvasBufferSize;           // Canvas buffer size

    // Playback state
    AnimationState _state;              // Current state
    LoopMode _loopMode;                 // Loop mode
    uint16_t _currentFrame;             // Current frame index
    bool _pingPongReverse;              // Ping-pong direction flag

    // Timing
    uint32_t _lastFrameTime;            // Last frame update time (ms)
    uint32_t _frameInterval;            // Time between frames (ms)
    float _speed;                       // Speed multiplier

    // Transform
    AnimationTransform _transform;      // Position, scale, opacity

    // Statistics
    AnimationStats _stats;              // Performance stats
    uint32_t _fpsStartTime;             // For FPS calculation
    uint16_t _fpsFrameCount;            // Frames since last FPS update
};

} // namespace Animation
} // namespace Doki

#endif // DOKI_ANIMATION_ANIMATION_PLAYER_H
