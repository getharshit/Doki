/**
 * Animation Test App
 *
 * Demonstrates the Animation API for Doki OS
 * Tests loading, playback control, and positioning
 */

var animId = -1;
var isPlaying = false;

function onCreate() {
    log("Animation Test App - Starting");

    // Set background
    setBackgroundColor(0x000000);

    // Create instructions
    createLabel("Animation Test", 120, 20);
    createLabel("Loading animation...", 120, 60);

    // Load test animation
    animId = loadAnimation("/animations/test.spr");

    if (animId >= 0) {
        log("Animation loaded successfully: ID=" + animId);

        // Position animation at center
        setAnimationPosition(animId, 70, 100);

        // Start playing with loop
        var success = playAnimation(animId, true);

        if (success) {
            isPlaying = true;
            updateLabel(1, "Playing (loop)");
            log("Animation playing");
        } else {
            updateLabel(1, "Failed to play");
            log("Failed to start animation");
        }
    } else {
        log("Failed to load animation");
        updateLabel(1, "Load failed!");
    }

    // Create control hints
    createLabel("Speed: 1.0x", 120, 200);
    createLabel("Opacity: 255", 120, 220);
}

var frameCount = 0;
var testPhase = 0;

function onUpdate() {
    frameCount++;

    // Update all animations
    updateAnimations();

    // Test different features every 3 seconds (90 frames @ 30fps)
    if (frameCount % 90 === 0 && animId >= 0) {
        testPhase++;

        switch (testPhase) {
            case 1:
                // Test speed change
                log("Testing speed: 2.0x");
                setAnimationSpeed(animId, 2.0);
                updateLabel(2, "Speed: 2.0x");
                break;

            case 2:
                // Test opacity change
                log("Testing opacity: 128");
                setAnimationOpacity(animId, 128);
                updateLabel(3, "Opacity: 128");
                break;

            case 3:
                // Test pause
                log("Testing pause");
                pauseAnimation(animId);
                isPlaying = false;
                updateLabel(1, "Paused");
                break;

            case 4:
                // Test resume
                log("Testing resume");
                resumeAnimation(animId);
                isPlaying = true;
                updateLabel(1, "Playing (loop)");
                break;

            case 5:
                // Reset to normal speed
                log("Reset to normal");
                setAnimationSpeed(animId, 1.0);
                setAnimationOpacity(animId, 255);
                updateLabel(2, "Speed: 1.0x");
                updateLabel(3, "Opacity: 255");
                break;

            case 6:
                // Test stop
                log("Testing stop");
                stopAnimation(animId);
                isPlaying = false;
                updateLabel(1, "Stopped");
                break;

            case 7:
                // Test replay
                log("Testing replay");
                playAnimation(animId, true);
                isPlaying = true;
                updateLabel(1, "Playing (loop)");
                testPhase = 0; // Loop back
                break;
        }
    }
}

function onPause() {
    log("Animation Test App - Paused");
    if (animId >= 0) {
        stopAnimation(animId);
    }
}

function onDestroy() {
    log("Animation Test App - Cleanup");
    if (animId >= 0) {
        unloadAnimation(animId);
    }
}
