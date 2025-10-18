/**
 * Cloud Weather Animation
 *
 * Displays the cloud weather animation centered on screen
 * with a looping playback.
 */

var cloudAnim = -1;

function onCreate() {
    log("Cloud Weather - Starting");

    // Safety: If animation already loaded, clean it up first
    if (cloudAnim >= 0) {
        log("WARNING: Animation already loaded, cleaning up...");
        stopAnimation(cloudAnim);
        unloadAnimation(cloudAnim);
        cloudAnim = -1;
    }

    // Clear screen first to remove any leftover UI elements
    clearScreen();

    // Black background
    setBackgroundColor(0x000000);

    // Load animation
    cloudAnim = loadAnimation("/animations/cloud_weather.spr");

    if (cloudAnim < 0) {
        log("ERROR: Failed to load cloud_weather.spr");
        createLabel("Animation failed to load", 120, 160);
        return;
    }

    // Center the 240×126 animation on 240×320 screen
    // Formula: x = (240 - 240) / 2 = 0 (full width)
    //          y = (320 - 126) / 2 = 97
    setAnimationPosition(cloudAnim, 0, 97);

    // Start playing (loop forever)
    var success = playAnimation(cloudAnim, true);

    if (!success) {
        log("ERROR: Failed to play animation");
    } else {
        log("Cloud animation playing");
    }
}

function onUpdate() {
    // Update all animations (required for playback)
    updateAnimations();
}

function onPause() {
    log("Cloud Weather - Paused");
    if (cloudAnim >= 0) {
        stopAnimation(cloudAnim);
    }
}

function onDestroy() {
    log("Cloud Weather - Cleanup");
    if (cloudAnim >= 0) {
        stopAnimation(cloudAnim);
        unloadAnimation(cloudAnim);
    }
}
