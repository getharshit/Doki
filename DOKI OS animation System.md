# Doki OS Animation System - Implementation Plan

## Current State Analysis

### What We Have
- ✅ LVGL graphics engine with 30+ FPS rendering
- ✅ 2MB PSRAM (currently using ~300KB for display buffers)
- ✅ JavaScript engine (Duktape) with LVGL bindings
- ✅ HTTP client for downloading data
- ✅ WebSocket support for real-time communication
- ✅ FreeRTOS task separation (Core 0: Network, Core 1: Rendering)
- ✅ File system (LittleFS) for storing apps

### What We Need
- ❌ Animation decoder/renderer in C++
- ❌ Server-side animation processor
- ❌ Animation memory management
- ❌ JavaScript API for animation control
- ❌ Sprite sheet loader and cache

## System Architecture

```
┌────────────────────────────────────────────────────────────────┐
│                     User Device (Phone/Web)                     │
└───────────────────────┬────────────────────────────────────────┘
                        │ Upload GIF/Lottie/SVG
                        ▼
┌────────────────────────────────────────────────────────────────┐
│                   Animation Processing Server                    │
│  ┌──────────────────────────────────────────────────────────┐  │
│  │   Processing Pipeline                                    │  │
│  │   • Format Detection (GIF/Lottie/SVG)                   │  │
│  │   • Frame Extraction & Optimization                      │  │
│  │   • Sprite Sheet Generation                             │  │
│  │   • Metadata Creation                                   │  │
│  └──────────────────────────────────────────────────────────┘  │
│  ┌──────────────────────────────────────────────────────────┐  │
│  │   Storage & CDN                                         │  │
│  │   • Processed Sprites (CDN)                             │  │
│  │   • Metadata Database                                   │  │
│  │   • Original Files Archive                              │  │
│  └──────────────────────────────────────────────────────────┘  │
└───────────────────────┬────────────────────────────────────────┘
                        │ HTTP/WebSocket
                        ▼
┌────────────────────────────────────────────────────────────────┐
│                         Doki OS (ESP32)                         │
│  ┌──────────────────────────────────────────────────────────┐  │
│  │   Animation Manager (C++)                NEW            │  │
│  │   • Sprite Sheet Loader                                 │  │
│  │   • Frame Buffer Management (PSRAM)                     │  │
│  │   • Playback Controller                                 │  │
│  │   • LVGL Integration                                    │  │
│  └────────────┬─────────────────────────────────────────────┘  │
│               │                                                 │
│  ┌────────────▼─────────────────────────────────────────────┐  │
│  │   JavaScript API Extension              UPDATE          │  │
│  │   • loadAnimation(url)                                  │  │
│  │   • playAnimation(id, x, y, options)                    │  │
│  │   • stopAnimation(id)                                   │  │
│  │   • onAnimationComplete(callback)                       │  │
│  └──────────────────────────────────────────────────────────┘  │
│                                                                 │
│  ┌──────────────────────────────────────────────────────────┐  │
│  │   Existing Systems                                      │  │
│  │   • LVGL Renderer (Core 1)                             │  │
│  │   • Network Manager (Core 0)                           │  │
│  │   • App Manager & JavaScript Engine                    │  │
│  └──────────────────────────────────────────────────────────┘  │
└────────────────────────────────────────────────────────────────┘
```

## Implementation Phases

### Phase 1: Core Animation Engine (Week 1)
**Goal**: Build C++ animation renderer for sprite sheets

#### 1.1 Memory Architecture
```
PSRAM Layout (2MB Total):
├── Display Buffer 1: 153KB (existing)
├── Display Buffer 2: 153KB (existing)
├── Animation Pool: 1MB (NEW)
│   ├── Sprite Cache: 800KB
│   │   ├── Animation 1: up to 400KB
│   │   └── Animation 2: up to 400KB
│   └── Metadata: 200KB
└── Free Space: ~700KB
```

#### 1.2 Core Classes
```cpp
AnimationManager (Singleton)
├── SpriteSheet class
│   ├── Load from file/memory
│   ├── Frame extraction
│   └── Memory management
├── Animation class
│   ├── Playback control
│   ├── Frame timing
│   └── LVGL canvas update
└── AnimationCache
    ├── LRU eviction
    └── Preloading
```

#### 1.3 Integration Points
- **LVGL**: Direct canvas manipulation for frames
- **FreeRTOS**: Timer task for frame updates (Core 1)
- **File System**: Load sprites from LittleFS
- **Network**: Download sprites via HTTP

#### Deliverables
- [ ] `include/animation/animation_manager.h`
- [ ] `src/animation/animation_manager.cpp`
- [ ] `include/animation/sprite_sheet.h`
- [ ] `src/animation/sprite_sheet.cpp`
- [ ] Test app that loads and plays a sample sprite sheet

### Phase 2: Server Processing Pipeline (Week 1-2)
**Goal**: Build server to convert animations to optimized sprites

#### 2.1 Server Architecture
```
Node.js/Python Server
├── API Layer (Express/FastAPI)
│   ├── /upload - Receive animation files
│   ├── /process - Convert to sprites
│   └── /download - Serve processed files
├── Processing Engine
│   ├── Format Detectors
│   ├── Frame Extractors
│   └── Optimization Pipeline
└── Storage Layer
    ├── Temporary processing
    ├── CDN integration
    └── Metadata database
```

#### 2.2 Processing Workflow
```
Input → Validate → Extract Frames → Optimize → 
Generate Sprite Sheet → Create Metadata → Store → Notify ESP32
```

#### 2.3 Optimization Parameters
- **Target Resolution**: 240x320 or smaller regions
- **Color Depth**: 8-bit indexed (256 colors)
- **Frame Rate**: Adaptive 12-30 FPS
- **Compression**: PNG with maximum compression

#### Deliverables
- [ ] Processing server setup (Node.js/Python)
- [ ] GIF to sprite converter
- [ ] Lottie to sprite converter
- [ ] REST API endpoints
- [ ] Basic web upload interface

### Phase 3: JavaScript API Integration (Week 2)
**Goal**: Expose animation system to JavaScript apps

#### 3.1 JavaScript Bindings
```javascript
// New Doki OS Animation API
class DokiAnimation {
  // Load animation from URL or local file
  static load(source, options)
  
  // Control playback
  play(loop = true)
  pause()
  stop()
  setSpeed(multiplier)
  
  // Positioning
  setPosition(x, y)
  setSize(width, height)
  
  // Events
  onComplete(callback)
  onFrame(callback)
  
  // Cleanup
  destroy()
}
```

#### 3.2 Duktape Bindings
```cpp
// Register functions with JavaScript engine
├── duk_animation_load()
├── duk_animation_play()
├── duk_animation_stop()
├── duk_animation_set_position()
└── duk_animation_destroy()
```

#### 3.3 Example App Usage
```javascript
// Weather app with animated icon
function WeatherApp() {
  var weatherAnim;
  
  this.onCreate = function() {
    // Load weather animation
    weatherAnim = DokiAnimation.load("/animations/sunny.spr");
    weatherAnim.setPosition(120, 100);
    weatherAnim.play();
  };
  
  this.onDestroy = function() {
    weatherAnim.destroy();
  };
}
```

#### Deliverables
- [ ] JavaScript API documentation
- [ ] Duktape bindings implementation
- [ ] Sample app with animations
- [ ] Memory leak testing

### Phase 4: Network Integration (Week 2-3)
**Goal**: Download and stream animations efficiently

#### 4.1 Download Manager
```cpp
AnimationDownloader
├── HTTP chunked download
├── Progress tracking
├── Resume capability
├── Background downloading
└── Cache management
```

#### 4.2 Streaming Protocol
```
ESP32 ← WebSocket ← Server
1. Request animation by ID
2. Receive metadata (size, frames, fps)
3. Stream chunks (4KB each)
4. Validate and store
5. Trigger playback
```

#### 4.3 Cache Strategy
- **Hot Cache**: Currently playing (in PSRAM)
- **Warm Cache**: Recently used (in LittleFS)
- **Cold Storage**: Server/CDN (download on demand)

#### Deliverables
- [ ] Animation download manager
- [ ] WebSocket streaming support
- [ ] Cache implementation
- [ ] Bandwidth optimization

### Phase 5: Multi-Display Support (Week 3)
**Goal**: Extend animation system for 3 displays

#### 5.1 Display Coordination
```cpp
DisplayAnimationManager
├── Display 0: Independent animations
├── Display 1: Independent animations
├── Display 2: Independent animations
└── Synchronization options
    ├── Play same animation
    ├── Play in sequence
    └── Coordinated animations
```

#### 5.2 Memory Management
```
1MB Animation Pool Division:
├── Display 0: 333KB
├── Display 1: 333KB
└── Display 2: 334KB
```

#### 5.3 JavaScript API Extension
```javascript
// Multi-display animation control
DokiAnimation.loadForDisplay(displayId, source);
DokiAnimation.synchronizeDisplays([0, 1, 2]);
```

#### Deliverables
- [ ] Multi-display animation manager
- [ ] Memory pool partitioning
- [ ] Synchronization logic
- [ ] JavaScript API updates

### Phase 6: Testing & Optimization (Week 3-4)
**Goal**: Ensure production-ready performance

#### 6.1 Performance Targets
- **Load Time**: < 2 seconds for 500KB sprite
- **Frame Rate**: Stable 24-30 FPS
- **Memory Usage**: < 1MB per animation
- **CPU Usage**: < 40% during playback
- **Multiple Animations**: 2 simultaneous without drops

#### 6.2 Test Suite
```
Test Cases:
├── Load/unload 1000 times (memory leaks)
├── Play for 24 hours (stability)
├── Network interruption (recovery)
├── Corrupt sprite data (error handling)
├── Multiple animations (performance)
└── JavaScript API stress test
```

#### 6.3 Optimizations
- Frame skipping for lag compensation
- Adaptive quality based on CPU load
- Predictive preloading
- Delta compression for similar frames

#### Deliverables
- [ ] Performance benchmark results
- [ ] Optimization report
- [ ] Test automation scripts
- [ ] Production configuration

## Memory Budget

### Current Usage
```
Internal RAM (512KB):
├── FreeRTOS & Core: ~150KB
├── Network Stack: ~100KB
├── JavaScript Heap: 128KB
├── LVGL Working: ~50KB
└── Free: ~84KB

PSRAM (2MB):
├── Display Buffers: 306KB
├── LVGL Cache: ~100KB
├── Animation Pool: 1MB (NEW)
└── Free: ~600KB
```

### Animation Memory Calculation
```
Single Animation:
- 120x120 pixels
- 8-bit color (1 byte/pixel)
- 30 frames
= 120 * 120 * 1 * 30 = 432KB

Full Screen Animation:
- 240x320 pixels
- 8-bit color
- 24 frames
= 240 * 320 * 1 * 24 = 1.8MB (needs compression)
```

## Risk Mitigation

### Technical Risks
1. **Memory Fragmentation**
   - Mitigation: Fixed-size memory pools
   
2. **Network Latency**
   - Mitigation: Progressive loading, caching

3. **CPU Overload**
   - Mitigation: Frame skipping, quality reduction

4. **Format Compatibility**
   - Mitigation: Server-side conversion handles all formats

### Implementation Risks
1. **Scope Creep**
   - Mitigation: Strict phase boundaries
   
2. **Integration Issues**
   - Mitigation: Incremental integration, extensive testing

## Success Metrics

### Phase 1 Complete When:
- [ ] Can load and play sprite sheet at 30 FPS
- [ ] Memory usage within 1MB budget
- [ ] No memory leaks after 100 plays

### Phase 2 Complete When:
- [ ] Server converts GIF/Lottie to sprites
- [ ] Processing time < 5 seconds
- [ ] Output optimized for ESP32

### Phase 3 Complete When:
- [ ] JavaScript apps can control animations
- [ ] Clean API with error handling
- [ ] Sample app working

### Phase 4 Complete When:
- [ ] Animations download over WiFi
- [ ] Streaming works reliably
- [ ] Cache prevents re-downloads

### Phase 5 Complete When:
- [ ] 3 displays show independent animations
- [ ] Synchronization options work
- [ ] Memory properly partitioned

### Phase 6 Complete When:
- [ ] All performance targets met
- [ ] 24-hour stability test passed
- [ ] Documentation complete

## Timeline Summary

**Week 1**: Core Animation Engine + Server Setup
**Week 2**: JavaScript API + Network Integration
**Week 3**: Multi-Display + Testing
**Week 4**: Optimization + Polish

**Total: 4 weeks to production-ready animation system**

## Next Immediate Steps

1. **Today**: Set up animation memory pool in PSRAM
2. **Tomorrow**: Create SpriteSheet class structure
3. **Day 3**: Implement frame extraction and timing
4. **Day 4**: LVGL canvas integration
5. **Day 5**: First sprite sheet playing at 30 FPS

Ready to start with Phase 1! 🚀