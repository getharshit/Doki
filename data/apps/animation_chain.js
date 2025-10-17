/**
 * Animation Chain Demo
 *
 * Demonstrates sequential animation playback - playing multiple
 * animations in a chain, one after another with smooth transitions.
 */

// Animation chain definition
// Screen: 240×320 pixels, Center point: (120, 160)
// Formula: x = (240 - width)/2, y = (320 - height)/2
var animations = [
    {name: "Spinner",   file: "/animations/spinner.spr",   duration: 2000, loop: true,  x: 70,  y: 110},  // 100×100: (240-100)/2=70, (320-100)/2=110
    {name: "Progress",  file: "/animations/progress.spr",  duration: 2000, loop: false, x: 30,  y: 152},  // 180×16: (240-180)/2=30, (320-16)/2=152
    {name: "Checkmark", file: "/animations/checkmark.spr", duration: 500,  loop: false, x: 80,  y: 120},  // 80×80: (240-80)/2=80, (320-80)/2=120
    {name: "Pulse",     file: "/animations/pulse.spr",     duration: 1500, loop: true,  x: 70,  y: 110}   // 100×100: (240-100)/2=70, (320-100)/2=110
];

// State tracking
var currentIndex = 0;
var currentAnimId = -1;
var startTime = 0;
var totalElapsed = 0;

// Labels
var errorLabel = -1;
var timeLabel = -1;

function onCreate() {
    log("Animation Chain Demo - Starting");

    // Set background to black
    setBackgroundColor(0x000000);

    // Create time display at bottom
    timeLabel = createLabel("Time: 0.0s", 120, 300);

    // Start first animation
    startNextAnimation();
}

function startNextAnimation() {
    // Stop current animation if playing
    if (currentAnimId >= 0) {
        stopAnimation(currentAnimId);
        unloadAnimation(currentAnimId);
        currentAnimId = -1;
    }

    // Check if we've completed all animations
    if (currentIndex >= animations.length) {
        log("Animation chain complete - restarting");
        currentIndex = 0;
        totalElapsed = 0;
    }

    var anim = animations[currentIndex];
    log("Starting animation: " + anim.name);

    // Load animation
    currentAnimId = loadAnimation(anim.file);

    if (currentAnimId < 0) {
        log("Failed to load: " + anim.file);

        // Show error at bottom
        if (errorLabel < 0) {
            errorLabel = createLabel("Error: " + anim.name, 120, 280);
        } else {
            updateLabel(errorLabel, "Error: " + anim.name);
        }

        // Skip to next animation
        currentIndex++;
        startNextAnimation();
        return;
    }

    // Clear any previous error
    if (errorLabel >= 0) {
        updateLabel(errorLabel, "");
    }

    // Position animation
    setAnimationPosition(currentAnimId, anim.x, anim.y);

    // Start playing
    var success = playAnimation(currentAnimId, anim.loop);

    if (!success) {
        log("Failed to play: " + anim.name);

        // Show error at bottom
        if (errorLabel < 0) {
            errorLabel = createLabel("Error playing: " + anim.name, 120, 280);
        } else {
            updateLabel(errorLabel, "Error playing: " + anim.name);
        }

        // Skip to next animation
        currentIndex++;
        startNextAnimation();
        return;
    }

    // Record start time
    startTime = millis();

    log("Animation playing: " + anim.name + " for " + anim.duration + "ms");
}

var frameCount = 0;

function onUpdate() {
    frameCount++;

    // Update all animations
    updateAnimations();

    // Update time display every 10 frames (~333ms @ 30fps)
    if (frameCount % 10 === 0) {
        var elapsed = (totalElapsed + (millis() - startTime)) / 1000;
        updateLabel(timeLabel, "Time: " + elapsed.toFixed(1) + "s");
    }

    // Check if current animation duration has elapsed
    if (currentAnimId >= 0) {
        var anim = animations[currentIndex];
        var elapsed = millis() - startTime;

        if (elapsed >= anim.duration) {
            log("Animation complete: " + anim.name + " (" + elapsed + "ms)");

            // Accumulate total time
            totalElapsed += elapsed;

            // Move to next animation
            currentIndex++;
            startNextAnimation();
        }
    }
}

function onPause() {
    log("Animation Chain - Paused");
    if (currentAnimId >= 0) {
        stopAnimation(currentAnimId);
    }
}

function onDestroy() {
    log("Animation Chain - Cleanup");
    if (currentAnimId >= 0) {
        stopAnimation(currentAnimId);
        unloadAnimation(currentAnimId);
    }
}
