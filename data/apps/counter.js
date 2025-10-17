/**
 * Counter JavaScript App for Doki OS
 *
 * Shows a simple counter that persists across app reloads.
 */

var count = 0;
var countLabel = null;

function onCreate() {
    log("Counter app created");

    // Dark blue background
    setBackgroundColor(0x001a33);

    // Title
    createLabel("Counter App", 60, 60);

    // Counter display (we'll update this in onUpdate)
    // Note: LVGL object references don't persist in JS yet,
    // so we recreate the label each time
    updateCounterDisplay();

    // Buttons
    createButton("+", 60, 180);
    createButton("-", 140, 180);
    createButton("Reset", 85, 220);
}

function onStart() {
    log("Counter app started");
}

function onUpdate() {
    // In a real implementation, you'd handle button clicks here
    // For now, we'll auto-increment every second as a demo
    count++;
    if (count > 999) count = 0;

    // Update display
    updateCounterDisplay();
}

function updateCounterDisplay() {
    // Create large counter label
    // In real LVGL binding, we'd update existing label instead
    createLabel("Count: " + count, 60, 120);
}

function onPause() {
    log("Counter paused at: " + count);
}

function onDestroy() {
    log("Counter destroyed");
}

function onSaveState() {
    log("Saving counter state: " + count);
    saveState("count", count.toString());
    return { count: count };
}

function onRestoreState(state) {
    var savedCount = loadState("count");
    if (savedCount) {
        count = parseInt(savedCount);
        log("Restored counter to: " + count);
    }
}
