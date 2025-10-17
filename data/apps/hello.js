/**
 * Hello World JavaScript App for Doki OS
 *
 * This is a simple example showing how to create a JavaScript app.
 * Place this file in /data/apps/hello.js on SPIFFS.
 */

// Called when app is created
function onCreate() {
    log("Hello World JS app created!");

    // Set background color (hex color code)
    setBackgroundColor(0x001133);

    // Create a label
    createLabel("Hello from", 60, 100);
    createLabel("JavaScript!", 50, 130);
    createLabel("👋", 100, 160);

    // Create a button
    createButton("Click Me", 60, 200);
}

// Called when app becomes visible
function onStart() {
    log("Hello World JS app started");
}

// Called every ~100ms while app is running
function onUpdate() {
    // Update logic here
    // Keep this lightweight!
}

// Called when app is paused
function onPause() {
    log("Hello World JS app paused");
}

// Called before app is destroyed
function onDestroy() {
    log("Goodbye from JavaScript!");
}

// Optional: Save app state
function onSaveState() {
    log("Saving state...");
    saveState("visits", "5");
    return { lastVisit: Date.now() };
}

// Optional: Restore app state
function onRestoreState(state) {
    log("Restoring state...");
    var visits = loadState("visits");
    log("Previous visits: " + visits);
}
