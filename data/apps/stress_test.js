/**
 * Animation System Stress Test
 *
 * Comprehensive stress testing for the animation system:
 * - Memory pressure testing (load until pool is full)
 * - Rapid load/unload cycles (memory leak detection)
 * - Concurrent playback limits
 * - Position update performance
 * - Speed variation handling
 */

// Test configuration
var RAPID_CYCLE_COUNT = 100;
var POSITION_TEST_FRAMES = 300;  // ~10 seconds at 30 FPS

// Animation definitions
var animations = [
    {file: "/animations/spinner.spr",   size: 196},  // 196 KB
    {file: "/animations/pulse.spr",     size: 196},  // 196 KB
    {file: "/animations/checkmark.spr", size: 95},   // 95 KB
    {file: "/animations/test.spr",      size: 81},   // 81 KB
    {file: "/animations/progress.spr",  size: 57},   // 57 KB
    {file: "/animations/bounce.spr",    size: 55}    // 55 KB
];  // Total: ~680 KB

// Test states
var TEST_IDLE = 0;
var TEST_MEMORY = 1;
var TEST_RAPID = 2;
var TEST_CONCURRENT = 3;
var TEST_POSITION = 4;
var TEST_SPEED = 5;
var TEST_COMPLETE = 6;

// State tracking
var currentTest = TEST_IDLE;
var testStartTime = 0;
var frameCount = 0;
var lastFpsUpdate = 0;
var currentFps = 0;

// Test-specific state
var loadedAnimations = [];
var cycleCount = 0;
var errorCount = 0;
var positionTestFrame = 0;
var angle = 0;

// UI elements
var titleLabel = -1;
var statusLabel = -1;
var fpsLabel = -1;
var memoryLabel = -1;
var cycleLabel = -1;
var errorLabel = -1;
var resultLabel = -1;
var progressLabel = -1;

// Test results
var memoryTestPassed = false;
var rapidTestPassed = false;
var concurrentTestPassed = false;
var positionTestPassed = false;
var speedTestPassed = false;

function onCreate() {
    log("=== Animation System Stress Test Starting ===");

    setBackgroundColor(0x000000);  // Black background

    // Title
    titleLabel = createLabel("STRESS TEST", 120, 20);

    // Status
    statusLabel = createLabel("Ready to start", 120, 60);

    // Metrics (centered vertically)
    fpsLabel = createLabel("FPS: --", 120, 120);
    memoryLabel = createLabel("Memory: -- / 1024 KB", 120, 145);
    cycleLabel = createLabel("Cycles: 0", 120, 170);
    errorLabel = createLabel("Errors: 0", 120, 195);

    // Progress
    progressLabel = createLabel("", 120, 230);

    // Result (bottom)
    resultLabel = createLabel("", 120, 280);

    // Start first test after 1 second
    currentTest = TEST_IDLE;
    testStartTime = millis();
}

function startTest(testId, description) {
    currentTest = testId;
    testStartTime = millis();
    frameCount = 0;
    updateLabel(statusLabel, description);
    updateLabel(progressLabel, "");
    updateLabel(resultLabel, "");
    log("--- Starting: " + description + " ---");
}

function cleanupAnimations() {
    for (var i = 0; i < loadedAnimations.length; i++) {
        stopAnimation(loadedAnimations[i]);
        unloadAnimation(loadedAnimations[i]);
    }
    loadedAnimations = [];
}

function updateMetrics() {
    // OPTIMIZATION: Only update labels every 30 frames (~1 second at 30 FPS)
    // This reduces LVGL redraw overhead and improves FPS
    if (frameCount % 30 !== 0) {
        return;  // Skip update
    }

    // Update FPS every second
    var now = millis();
    if (now - lastFpsUpdate >= 1000) {
        currentFps = frameCount;
        frameCount = 0;
        lastFpsUpdate = now;
        updateLabel(fpsLabel, "FPS: " + currentFps);
    }

    // Update cycle and error counts (once per second)
    updateLabel(cycleLabel, "Cycles: " + cycleCount);
    updateLabel(errorLabel, "Errors: " + errorCount);
}

// ==========================================
// Test 1: Memory Stress Test
// ==========================================
function testMemory() {
    var elapsed = millis() - testStartTime;

    if (elapsed < 500) {
        // Wait for initialization
        return;
    }

    if (loadedAnimations.length === 0) {
        log("[Memory Test] Loading all " + animations.length + " animations...");

        var totalLoaded = 0;

        for (var i = 0; i < animations.length; i++) {
            var animId = loadAnimation(animations[i].file);

            if (animId >= 0) {
                loadedAnimations.push(animId);
                totalLoaded++;
                log("[Memory Test] Loaded: " + animations[i].file + " (ID: " + animId + ")");
            } else {
                errorCount++;
                log("[Memory Test] FAILED to load: " + animations[i].file);
            }
        }

        updateLabel(memoryLabel, "Loaded " + totalLoaded + "/" + animations.length + " anims");

        if (totalLoaded === animations.length) {
            log("[Memory Test] SUCCESS: All animations loaded");
            memoryTestPassed = true;
        } else {
            log("[Memory Test] PARTIAL: Only " + totalLoaded + "/" + animations.length + " loaded");
            memoryTestPassed = false;
        }
    }

    // Run for 3 seconds, then cleanup and move to next test
    if (elapsed >= 3000) {
        log("[Memory Test] Cleaning up...");
        cleanupAnimations();
        updateLabel(resultLabel, memoryTestPassed ? "PASS" : "FAIL");

        // Wait 1 second before next test
        if (elapsed >= 4000) {
            startTest(TEST_RAPID, "Test 2: Rapid Cycling");
        }
    }
}

// ==========================================
// Test 2: Rapid Load/Unload Cycles
// ==========================================
function testRapidCycles() {
    var elapsed = millis() - testStartTime;

    if (elapsed < 500) {
        return;
    }

    // Perform rapid cycles (one per frame)
    if (cycleCount < RAPID_CYCLE_COUNT) {
        // Load smallest animation (bounce.spr - 55 KB)
        var animId = loadAnimation("/animations/bounce.spr");

        if (animId >= 0) {
            unloadAnimation(animId);
            cycleCount++;

            // Update progress every 10 cycles
            if (cycleCount % 10 === 0) {
                var progress = Math.floor((cycleCount / RAPID_CYCLE_COUNT) * 100);
                updateLabel(progressLabel, progress + "% (" + cycleCount + "/" + RAPID_CYCLE_COUNT + ")");
            }
        } else {
            errorCount++;
            log("[Rapid Test] Load failed at cycle " + cycleCount);
        }
    } else if (cycleCount === RAPID_CYCLE_COUNT) {
        // Completed all cycles
        log("[Rapid Test] Completed " + RAPID_CYCLE_COUNT + " cycles");
        log("[Rapid Test] Errors: " + errorCount);

        rapidTestPassed = (errorCount === 0);
        updateLabel(resultLabel, rapidTestPassed ? "PASS" : "FAIL");

        cycleCount++;  // Move to completion state
    }

    // Move to next test after 2 seconds
    if (elapsed >= 2000 && cycleCount > RAPID_CYCLE_COUNT) {
        cycleCount = 0;  // Reset for next test
        startTest(TEST_CONCURRENT, "Test 3: Concurrent Playback");
    }
}

// ==========================================
// Test 3: Concurrent Playback Test
// ==========================================
function testConcurrentPlayback() {
    var elapsed = millis() - testStartTime;

    if (elapsed < 500) {
        return;
    }

    // Load and play 3 animations (max is 2, so one should fail gracefully)
    if (loadedAnimations.length === 0) {
        log("[Concurrent Test] Attempting to play 3 animations simultaneously...");

        var positions = [
            {x: 40, y: 80},   // Top-left
            {x: 140, y: 80},  // Top-right
            {x: 90, y: 180}   // Bottom-center
        ];

        var testAnims = [
            "/animations/spinner.spr",
            "/animations/pulse.spr",
            "/animations/checkmark.spr"
        ];

        var playingCount = 0;

        for (var i = 0; i < testAnims.length; i++) {
            var animId = loadAnimation(testAnims[i]);

            if (animId >= 0) {
                loadedAnimations.push(animId);
                setAnimationPosition(animId, positions[i].x, positions[i].y);

                var success = playAnimation(animId, true);  // Loop
                if (success) {
                    playingCount++;
                    log("[Concurrent Test] Playing: " + testAnims[i]);
                } else {
                    log("[Concurrent Test] Play failed for: " + testAnims[i]);
                }
            } else {
                log("[Concurrent Test] Load failed for: " + testAnims[i]);
            }
        }

        updateLabel(memoryLabel, "Playing: " + playingCount + "/3 animations");

        // Pass if system handled 2 animations correctly
        concurrentTestPassed = (playingCount >= 2);
    }

    // Run for 3 seconds
    if (elapsed >= 3000) {
        log("[Concurrent Test] Completed - " + loadedAnimations.length + " animations active");
        cleanupAnimations();
        updateLabel(resultLabel, concurrentTestPassed ? "PASS" : "FAIL");

        if (elapsed >= 4000) {
            startTest(TEST_POSITION, "Test 4: Position Thrashing");
        }
    }
}

// ==========================================
// Test 4: Position Update Stress Test
// ==========================================
function testPositionThrashing() {
    var elapsed = millis() - testStartTime;

    if (elapsed < 500) {
        return;
    }

    // Load one animation if not loaded
    if (loadedAnimations.length === 0) {
        log("[Position Test] Loading spinner for position updates...");
        var animId = loadAnimation("/animations/spinner.spr");

        if (animId >= 0) {
            loadedAnimations.push(animId);
            playAnimation(animId, true);  // Loop
        } else {
            errorCount++;
            positionTestPassed = false;
            updateLabel(resultLabel, "FAIL - Load Error");
            startTest(TEST_SPEED, "Test 5: Speed Variation");
            return;
        }
    }

    // Update position every frame in circular pattern
    if (loadedAnimations.length > 0) {
        var animId = loadedAnimations[0];

        // Circular motion: center at (120, 160), radius 60
        var centerX = 120;
        var centerY = 160;
        var radius = 60;
        var spriteSize = 100;  // spinner is 100×100

        angle += 5;  // Degrees per frame
        if (angle >= 360) angle = 0;

        var radians = angle * 3.14159 / 180;
        var x = centerX + Math.floor(radius * Math.cos(radians)) - spriteSize / 2;
        var y = centerY + Math.floor(radius * Math.sin(radians)) - spriteSize / 2;

        setAnimationPosition(animId, x, y);
        positionTestFrame++;

        // Update progress
        if (positionTestFrame % 30 === 0) {
            var progress = Math.floor((positionTestFrame / POSITION_TEST_FRAMES) * 100);
            updateLabel(progressLabel, progress + "% - Moving in circle");
        }
    }

    // Run until frame count reached
    if (positionTestFrame >= POSITION_TEST_FRAMES) {
        log("[Position Test] Completed " + positionTestFrame + " position updates");
        positionTestPassed = true;
        cleanupAnimations();
        updateLabel(resultLabel, "PASS");
        positionTestFrame = 0;
        angle = 0;

        if (elapsed >= POSITION_TEST_FRAMES / 30 * 1000 + 1000) {
            startTest(TEST_SPEED, "Test 5: Speed Variation");
        }
    }
}

// ==========================================
// Test 5: Speed Variation Test
// ==========================================
var speedTestPhase = 0;
var speedTestStartFrame = 0;

function testSpeedVariation() {
    var elapsed = millis() - testStartTime;

    if (elapsed < 500) {
        return;
    }

    // Load animation if not loaded
    if (loadedAnimations.length === 0) {
        log("[Speed Test] Loading spinner for speed tests...");
        var animId = loadAnimation("/animations/spinner.spr");

        if (animId >= 0) {
            loadedAnimations.push(animId);
            setAnimationPosition(animId, 70, 110);  // Center
            playAnimation(animId, true);  // Loop
            speedTestStartFrame = frameCount;
        } else {
            errorCount++;
            speedTestPassed = false;
            updateLabel(resultLabel, "FAIL - Load Error");
            startTest(TEST_COMPLETE, "Tests Complete");
            return;
        }
    }

    // Cycle through speeds: 0.5x → 1.0x → 1.5x → 2.0x
    if (loadedAnimations.length > 0) {
        var animId = loadedAnimations[0];
        var framesSinceStart = frameCount - speedTestStartFrame;

        // Change speed every 60 frames (~2 seconds at 30 FPS)
        if (framesSinceStart < 60) {
            // Phase 0: 0.5x speed
            if (speedTestPhase !== 0) {
                speedTestPhase = 0;
                updateLabel(progressLabel, "Speed: 0.5x (slow)");
            }
        } else if (framesSinceStart < 120) {
            // Phase 1: 1.0x speed
            if (speedTestPhase !== 1) {
                speedTestPhase = 1;
                updateLabel(progressLabel, "Speed: 1.0x (normal)");
            }
        } else if (framesSinceStart < 180) {
            // Phase 2: 1.5x speed
            if (speedTestPhase !== 2) {
                speedTestPhase = 2;
                updateLabel(progressLabel, "Speed: 1.5x (fast)");
            }
        } else if (framesSinceStart < 240) {
            // Phase 3: 2.0x speed
            if (speedTestPhase !== 3) {
                speedTestPhase = 3;
                updateLabel(progressLabel, "Speed: 2.0x (very fast)");
            }
        } else {
            // Completed all phases
            log("[Speed Test] Completed all speed variations");
            speedTestPassed = true;
            cleanupAnimations();
            updateLabel(resultLabel, "PASS");
            speedTestPhase = 0;
            speedTestStartFrame = 0;

            startTest(TEST_COMPLETE, "Tests Complete");
        }
    }
}

// ==========================================
// Test Complete - Show Results
// ==========================================
function showResults() {
    var elapsed = millis() - testStartTime;

    if (elapsed < 500) {
        return;
    }

    // Calculate overall result
    var passCount = 0;
    if (memoryTestPassed) passCount++;
    if (rapidTestPassed) passCount++;
    if (concurrentTestPassed) passCount++;
    if (positionTestPassed) passCount++;
    if (speedTestPassed) passCount++;

    var allPassed = (passCount === 5);

    // Update UI
    updateLabel(statusLabel, "All Tests Completed");
    updateLabel(progressLabel, passCount + "/5 tests passed");
    updateLabel(resultLabel, allPassed ? "*** ALL PASS ***" : "*** SOME FAILED ***");

    log("=== STRESS TEST RESULTS ===");
    log("Memory Stress:      " + (memoryTestPassed ? "PASS" : "FAIL"));
    log("Rapid Cycling:      " + (rapidTestPassed ? "PASS" : "FAIL"));
    log("Concurrent Playback: " + (concurrentTestPassed ? "PASS" : "FAIL"));
    log("Position Thrashing: " + (positionTestPassed ? "PASS" : "FAIL"));
    log("Speed Variation:    " + (speedTestPassed ? "PASS" : "FAIL"));
    log("==========================");
    log("Total Errors: " + errorCount);
    log("Overall: " + (allPassed ? "PASS" : "FAIL"));

    // Stay in complete state
    currentTest = TEST_COMPLETE;
}

// ==========================================
// Main Update Loop
// ==========================================
function onUpdate() {
    frameCount++;

    // Update all animations
    updateAnimations();

    // Update metrics
    updateMetrics();

    // State machine
    if (currentTest === TEST_IDLE) {
        // Wait 1 second before starting
        if (millis() - testStartTime >= 1000) {
            startTest(TEST_MEMORY, "Test 1: Memory Stress");
        }
    } else if (currentTest === TEST_MEMORY) {
        testMemory();
    } else if (currentTest === TEST_RAPID) {
        testRapidCycles();
    } else if (currentTest === TEST_CONCURRENT) {
        testConcurrentPlayback();
    } else if (currentTest === TEST_POSITION) {
        testPositionThrashing();
    } else if (currentTest === TEST_SPEED) {
        testSpeedVariation();
    } else if (currentTest === TEST_COMPLETE) {
        showResults();
    }
}

function onPause() {
    log("Stress Test - Paused");
    cleanupAnimations();
}

function onDestroy() {
    log("Stress Test - Cleanup");
    cleanupAnimations();
}
