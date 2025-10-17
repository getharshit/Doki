/**
 * @file animation_player.cpp
 * @brief Implementation of AnimationPlayer class
 */

#include "doki/animation/animation_player.h"
#include "doki/lvgl_manager.h"
#include <esp_heap_caps.h>

namespace Doki {
namespace Animation {

// ==========================================
// Constructor / Destructor
// ==========================================

AnimationPlayer::AnimationPlayer(SpriteSheet* sprite, lv_obj_t* parent)
    : _sprite(sprite),
      _parent(parent),
      _canvas(nullptr),
      _canvasBuffer(nullptr),
      _canvasBufferSize(0),
      _state(AnimationState::IDLE),
      _loopMode(LoopMode::ONCE),
      _currentFrame(0),
      _pingPongReverse(false),
      _lastFrameTime(0),
      _frameInterval(UPDATE_INTERVAL_ANIMATION_MS),
      _speed(1.0f),
      _fpsStartTime(0),
      _fpsFrameCount(0) {

    memset(&_imgDsc, 0, sizeof(lv_img_dsc_t));

    if (!_sprite || !_sprite->isLoaded()) {
        Serial.println("[AnimationPlayer] Error: Invalid or unloaded sprite");
        _state = AnimationState::ERROR;
        return;
    }

    // Create canvas
    if (!createCanvas()) {
        Serial.println("[AnimationPlayer] Error: Failed to create canvas");
        _state = AnimationState::ERROR;
        return;
    }

    // Calculate frame interval from sprite FPS
    _frameInterval = fpsToInterval(_sprite->getFPS());

    Serial.printf("[AnimationPlayer] Created (%dx%d, %d frames, %d FPS)\n",
                 _sprite->getFrameWidth(), _sprite->getFrameHeight(),
                 _sprite->getFrameCount(), _sprite->getFPS());

    _state = AnimationState::LOADED;
}

AnimationPlayer::~AnimationPlayer() {
    Serial.println("[AnimationPlayer] Destroying...");

    // Delete canvas
    if (_canvas) {
        lv_obj_del(_canvas);
        _canvas = nullptr;
    }

    // Free canvas buffer
    if (_canvasBuffer) {
        heap_caps_free(_canvasBuffer);
        _canvasBuffer = nullptr;
    }
}

// ==========================================
// Playback Control
// ==========================================

bool AnimationPlayer::play(LoopMode mode) {
    if (_state == AnimationState::ERROR) {
        Serial.println("[AnimationPlayer] Error: Cannot play in error state");
        return false;
    }

    _loopMode = mode;
    _state = AnimationState::PLAYING;
    _currentFrame = 0;
    _pingPongReverse = false;
    _lastFrameTime = millis();

    // Reset statistics
    _stats.framesPlayed = 0;
    _stats.framesDropped = 0;
    _fpsStartTime = millis();
    _fpsFrameCount = 0;

    // Update canvas with first frame
    updateCanvas();

    Serial.printf("[AnimationPlayer] ▶ Playing (mode=%d)\n", (int)mode);
    return true;
}

void AnimationPlayer::pause() {
    if (_state == AnimationState::PLAYING) {
        _state = AnimationState::PAUSED;
        Serial.println("[AnimationPlayer] ⏸ Paused");
    }
}

void AnimationPlayer::resume() {
    if (_state == AnimationState::PAUSED) {
        _state = AnimationState::PLAYING;
        _lastFrameTime = millis();  // Reset timing
        Serial.println("[AnimationPlayer] ▶ Resumed");
    }
}

void AnimationPlayer::stop() {
    _state = AnimationState::LOADED;
    _currentFrame = 0;
    _pingPongReverse = false;

    // Update canvas with first frame
    updateCanvas();

    Serial.println("[AnimationPlayer] ⏹ Stopped");
}

bool AnimationPlayer::update() {
    if (_state != AnimationState::PLAYING) {
        return false;
    }

    // Check if should advance frame
    if (!shouldAdvanceFrame()) {
        return false;
    }

    // Calculate next frame
    uint16_t nextFrame = calculateNextFrame();

    // Check for end of animation (non-looping)
    if (_loopMode == LoopMode::ONCE && nextFrame == 0 && _currentFrame == getFrameCount() - 1) {
        stop();
        return false;
    }

    _currentFrame = nextFrame;
    _lastFrameTime = millis();

    // Update canvas
    updateCanvas();

    // Update statistics
    _stats.framesPlayed++;
    _fpsFrameCount++;

    // Calculate FPS every second
    uint32_t now = millis();
    if (now - _fpsStartTime >= 1000) {
        _stats.avgFps = _fpsFrameCount * 1000.0f / (now - _fpsStartTime);
        _fpsFrameCount = 0;
        _fpsStartTime = now;
    }

    return true;
}

// ==========================================
// Playback Settings
// ==========================================

void AnimationPlayer::setSpeed(float speed) {
    if (speed < 0.1f) speed = 0.1f;
    if (speed > 10.0f) speed = 10.0f;

    _speed = speed;
    Serial.printf("[AnimationPlayer] Speed set to %.2fx\n", speed);
}

// ==========================================
// Position & Transform
// ==========================================

void AnimationPlayer::setPosition(int16_t x, int16_t y) {
    _transform.x = x;
    _transform.y = y;

    if (_canvas) {
        lv_obj_set_pos(_canvas, x, y);
    }
}

void AnimationPlayer::setOpacity(uint8_t opacity) {
    _transform.opacity = opacity;

    if (_canvas) {
        lv_obj_set_style_opa(_canvas, opacity, 0);
    }
}

void AnimationPlayer::setVisible(bool visible) {
    if (_canvas) {
        if (visible) {
            lv_obj_clear_flag(_canvas, LV_OBJ_FLAG_HIDDEN);
        } else {
            lv_obj_add_flag(_canvas, LV_OBJ_FLAG_HIDDEN);
        }
    }
}

bool AnimationPlayer::isVisible() const {
    if (!_canvas) return false;
    return !lv_obj_has_flag(_canvas, LV_OBJ_FLAG_HIDDEN);
}

// ==========================================
// Frame Control
// ==========================================

void AnimationPlayer::gotoFrame(uint16_t frameIndex) {
    if (frameIndex >= getFrameCount()) {
        return;
    }

    _currentFrame = frameIndex;
    updateCanvas();
}

void AnimationPlayer::nextFrame() {
    uint16_t next = _currentFrame + 1;
    if (next >= getFrameCount()) next = 0;
    gotoFrame(next);
}

void AnimationPlayer::prevFrame() {
    uint16_t prev = _currentFrame;
    if (prev == 0) prev = getFrameCount() - 1;
    else prev--;
    gotoFrame(prev);
}

// ==========================================
// Status
// ==========================================

void AnimationPlayer::resetStats() {
    _stats.framesPlayed = 0;
    _stats.framesDropped = 0;
    _stats.avgFps = 0.0f;
    _fpsStartTime = millis();
    _fpsFrameCount = 0;
}

// ==========================================
// Internal Methods
// ==========================================

bool AnimationPlayer::createCanvas() {
    if (!_sprite || !_sprite->isLoaded()) {
        return false;
    }

    uint16_t width = _sprite->getFrameWidth();
    uint16_t height = _sprite->getFrameHeight();

    // Allocate canvas buffer in PSRAM (RGB565 format)
    _canvasBufferSize = width * height * 2;  // 2 bytes per pixel (RGB565)
    _canvasBuffer = (uint16_t*)heap_caps_malloc(_canvasBufferSize, MALLOC_CAP_SPIRAM);

    if (!_canvasBuffer) {
        Serial.println("[AnimationPlayer] Error: Failed to allocate canvas buffer");
        return false;
    }

    // Clear buffer
    memset(_canvasBuffer, 0, _canvasBufferSize);

    // Create LVGL canvas (with mutex protection)
    Doki::LVGLManager::lock();
    _canvas = lv_canvas_create(_parent);
    if (!_canvas) {
        Doki::LVGLManager::unlock();
        Serial.println("[AnimationPlayer] Error: Failed to create LVGL canvas");
        heap_caps_free(_canvasBuffer);
        _canvasBuffer = nullptr;
        return false;
    }

    // Set canvas buffer
    lv_canvas_set_buffer(_canvas, _canvasBuffer, width, height, LV_IMG_CF_TRUE_COLOR);

    // Set position
    lv_obj_set_pos(_canvas, _transform.x, _transform.y);
    Doki::LVGLManager::unlock();

    Serial.printf("[AnimationPlayer] Canvas created (%dx%d, %zu bytes)\n",
                 width, height, _canvasBufferSize);

    _stats.memoryUsed = _canvasBufferSize;

    return true;
}

void AnimationPlayer::updateCanvas() {
    if (!_sprite || !_canvasBuffer) {
        return;
    }

    // Get current frame data
    const uint8_t* frameData = _sprite->getFrameData(_currentFrame);
    if (!frameData) {
        return;
    }

    // Convert frame to RGB565 and update canvas
    if (_sprite->getColorFormat() == ColorFormat::INDEXED_8BIT) {
        convertFrameToRGB565(frameData, _canvasBuffer);
    } else {
        // Direct copy for RGB565
        size_t frameSize = _sprite->getFrameSize(_currentFrame);
        memcpy(_canvasBuffer, frameData, frameSize);
    }

    // Invalidate canvas to trigger redraw (with mutex protection)
    if (_canvas) {
        Doki::LVGLManager::lock();
        lv_obj_invalidate(_canvas);
        Doki::LVGLManager::unlock();
    }
}

void AnimationPlayer::convertFrameToRGB565(const uint8_t* frameData, uint16_t* output) {
    if (!frameData || !output) {
        return;
    }

    const RGBAColor* palette = _sprite->getPalette();
    if (!palette) {
        return;
    }

    uint16_t width = _sprite->getFrameWidth();
    uint16_t height = _sprite->getFrameHeight();
    size_t pixelCount = width * height;

    // Convert each pixel from indexed to RGB565
    for (size_t i = 0; i < pixelCount; i++) {
        uint8_t index = frameData[i];
        const RGBAColor& color = palette[index];

        // Convert RGBA to RGB565
        uint16_t r = (color.r >> 3) & 0x1F;      // 5 bits red
        uint16_t g = (color.g >> 2) & 0x3F;      // 6 bits green
        uint16_t b = (color.b >> 3) & 0x1F;      // 5 bits blue

        output[i] = (r << 11) | (g << 5) | b;

        // Apply opacity (simple alpha blending with black background)
        if (color.a < 255) {
            uint8_t alpha = color.a;
            uint16_t bgColor = 0x0000;  // Black background

            // Blend with background
            uint16_t fgR = (output[i] >> 11) & 0x1F;
            uint16_t fgG = (output[i] >> 5) & 0x3F;
            uint16_t fgB = output[i] & 0x1F;

            uint16_t bgR = (bgColor >> 11) & 0x1F;
            uint16_t bgG = (bgColor >> 5) & 0x3F;
            uint16_t bgB = bgColor & 0x1F;

            uint16_t blendR = (fgR * alpha + bgR * (255 - alpha)) / 255;
            uint16_t blendG = (fgG * alpha + bgG * (255 - alpha)) / 255;
            uint16_t blendB = (fgB * alpha + bgB * (255 - alpha)) / 255;

            output[i] = (blendR << 11) | (blendG << 5) | blendB;
        }
    }
}

uint16_t AnimationPlayer::calculateNextFrame() {
    uint16_t frameCount = getFrameCount();
    uint16_t next = _currentFrame;

    switch (_loopMode) {
        case LoopMode::ONCE:
        case LoopMode::LOOP:
            next = (_currentFrame + 1) % frameCount;
            break;

        case LoopMode::PING_PONG:
            if (_pingPongReverse) {
                if (_currentFrame == 0) {
                    _pingPongReverse = false;
                    next = 1;
                } else {
                    next = _currentFrame - 1;
                }
            } else {
                if (_currentFrame == frameCount - 1) {
                    _pingPongReverse = true;
                    next = frameCount - 2;
                } else {
                    next = _currentFrame + 1;
                }
            }
            break;
    }

    return next;
}

bool AnimationPlayer::shouldAdvanceFrame() {
    uint32_t now = millis();
    uint32_t elapsed = now - _lastFrameTime;

    // Calculate adjusted interval based on speed
    uint32_t adjustedInterval = _frameInterval / _speed;

    return (elapsed >= adjustedInterval);
}

} // namespace Animation
} // namespace Doki
