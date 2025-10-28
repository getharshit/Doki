/**
 * LVGL UI Helpers for Doki OS
 *
 * Common patterns for creating UI elements with consistent styling.
 * Reduces boilerplate code in apps and ensures visual consistency.
 *
 * Usage Example:
 *   lv_obj_t* label = createStyledLabel(screen, "Hello",
 *                                       0xFF0000, &lv_font_montserrat_24,
 *                                       LV_ALIGN_CENTER, 0, 0);
 */

#ifndef DOKI_LVGL_HELPERS_H
#define DOKI_LVGL_HELPERS_H

#include <lvgl.h>

namespace Doki {

/**
 * Create a styled label with common settings
 *
 * @param parent Parent object (usually screen)
 * @param text Initial text content
 * @param color Text color (RGB hex, e.g., 0xFF0000 for red)
 * @param font Font to use (e.g., &lv_font_montserrat_24)
 * @param align Alignment type (e.g., LV_ALIGN_CENTER)
 * @param x_offset X offset from alignment point
 * @param y_offset Y offset from alignment point
 * @return Pointer to created label
 */
inline lv_obj_t* createStyledLabel(lv_obj_t* parent, const char* text,
                                   uint32_t color, const lv_font_t* font,
                                   lv_align_t align, int x_offset, int y_offset) {
    lv_obj_t* label = lv_label_create(parent);
    lv_label_set_text(label, text);
    lv_obj_align(label, align, x_offset, y_offset);
    lv_obj_set_style_text_font(label, font, 0);
    lv_obj_set_style_text_color(label, lv_color_hex(color), 0);
    return label;
}

/**
 * Create a simple label (default font and color)
 */
inline lv_obj_t* createLabel(lv_obj_t* parent, const char* text,
                             lv_align_t align, int x_offset, int y_offset) {
    return createStyledLabel(parent, text, 0xFFFFFF,
                            &lv_font_montserrat_14, align, x_offset, y_offset);
}

/**
 * Create a title label (large, colored)
 */
inline lv_obj_t* createTitleLabel(lv_obj_t* parent, const char* text,
                                  uint32_t color, int y_offset) {
    return createStyledLabel(parent, text, color,
                            &lv_font_montserrat_24,
                            LV_ALIGN_TOP_MID, 0, y_offset);
}

/**
 * Create a centered value label (large font)
 */
inline lv_obj_t* createValueLabel(lv_obj_t* parent, const char* text,
                                  uint32_t color, int y_offset) {
    return createStyledLabel(parent, text, color,
                            &lv_font_montserrat_48,
                            LV_ALIGN_CENTER, 0, y_offset);
}

/**
 * Create a small info label
 */
inline lv_obj_t* createInfoLabel(lv_obj_t* parent, const char* text,
                                 lv_align_t align, int x_offset, int y_offset) {
    return createStyledLabel(parent, text, 0x888888,
                            &lv_font_montserrat_10, align, x_offset, y_offset);
}

/**
 * Create a styled progress bar
 *
 * @param parent Parent object
 * @param width Bar width in pixels
 * @param height Bar height in pixels
 * @param bg_color Background color (RGB hex)
 * @param fg_color Foreground/indicator color (RGB hex)
 * @param align Alignment
 * @param x_offset X offset
 * @param y_offset Y offset
 * @return Pointer to created bar
 */
inline lv_obj_t* createProgressBar(lv_obj_t* parent, int width, int height,
                                   uint32_t bg_color, uint32_t fg_color,
                                   lv_align_t align, int x_offset, int y_offset) {
    lv_obj_t* bar = lv_bar_create(parent);
    lv_obj_set_size(bar, width, height);
    lv_obj_align(bar, align, x_offset, y_offset);
    lv_bar_set_range(bar, 0, 100);
    lv_obj_set_style_bg_color(bar, lv_color_hex(bg_color), 0);
    lv_obj_set_style_bg_color(bar, lv_color_hex(fg_color), LV_PART_INDICATOR);
    lv_obj_set_style_radius(bar, 10, 0);
    return bar;
}

/**
 * Create a horizontal line separator
 */
inline void createSeparator(lv_obj_t* parent, int y_offset,
                           int width = 220, uint32_t color = 0x333333) {
    lv_obj_t* line = lv_obj_create(parent);
    lv_obj_set_size(line, width, 2);
    lv_obj_align(line, LV_ALIGN_TOP_MID, 0, y_offset);
    lv_obj_set_style_bg_color(line, lv_color_hex(color), 0);
    lv_obj_set_style_border_width(line, 0, 0);
    lv_obj_set_style_radius(line, 0, 0);
}

/**
 * Create a small decorative circle
 */
inline void createDot(lv_obj_t* parent, int x, int y,
                     int radius = 3, uint32_t color = 0x00d4ff) {
    lv_obj_t* dot = lv_obj_create(parent);
    lv_obj_set_size(dot, radius * 2, radius * 2);
    lv_obj_align(dot, LV_ALIGN_TOP_LEFT, x, y);
    lv_obj_set_style_bg_color(dot, lv_color_hex(color), 0);
    lv_obj_set_style_radius(dot, radius, 0);
    lv_obj_set_style_border_width(dot, 0, 0);
}

/**
 * Set background color for screen/container
 */
inline void setBackgroundColor(lv_obj_t* obj, uint32_t color) {
    lv_obj_set_style_bg_color(obj, lv_color_hex(color), 0);
}

/**
 * Update label text (convenience wrapper)
 */
inline void updateLabelText(lv_obj_t* label, const char* text) {
    if (label) {
        lv_label_set_text(label, text);
    }
}

/**
 * Update label color
 */
inline void updateLabelColor(lv_obj_t* label, uint32_t color) {
    if (label) {
        lv_obj_set_style_text_color(label, lv_color_hex(color), 0);
    }
}

/**
 * Update progress bar value (0-100)
 */
inline void updateProgressBar(lv_obj_t* bar, int value, bool animate = true) {
    if (bar) {
        lv_bar_set_value(bar, value, animate ? LV_ANIM_ON : LV_ANIM_OFF);
    }
}

/**
 * Apply fade-in animation to object
 */
inline void applyFadeIn(lv_obj_t* obj, uint32_t duration_ms) {
    if (obj) {
        lv_obj_set_style_opa(obj, LV_OPA_TRANSP, 0);  // Start transparent
        lv_obj_fade_in(obj, duration_ms, 0);
    }
}

/**
 * Apply fade-out animation to object
 */
inline void applyFadeOut(lv_obj_t* obj, uint32_t duration_ms) {
    if (obj) {
        lv_obj_fade_out(obj, duration_ms, 0);
    }
}

/**
 * Common color palette
 */
namespace Colors {
    constexpr uint32_t WHITE       = 0xFFFFFF;
    constexpr uint32_t BLACK       = 0x000000;
    constexpr uint32_t GRAY        = 0x888888;
    constexpr uint32_t LIGHT_GRAY  = 0xe5e7eb;
    constexpr uint32_t DARK_GRAY   = 0x333333;

    constexpr uint32_t RED         = 0xFF0000;
    constexpr uint32_t GREEN       = 0x00FF00;
    constexpr uint32_t BLUE        = 0x0000FF;

    constexpr uint32_t CYAN        = 0x00d4ff;
    constexpr uint32_t ORANGE      = 0xFF6B35;
    constexpr uint32_t YELLOW      = 0xffaa00;
    constexpr uint32_t LIME        = 0x00ff88;

    constexpr uint32_t SUCCESS     = 0x10b981;  // Green
    constexpr uint32_t WARNING     = 0xf59e0b;  // Amber
    constexpr uint32_t ERROR       = 0xff4444;  // Red
    constexpr uint32_t INFO        = 0x3B82F6;  // Blue

    constexpr uint32_t BACKGROUND  = 0x0a0e27;  // Dark blue-gray

    // Clock face colors
    constexpr uint32_t NEON_YELLOW = 0xFFED4E;  // Bright yellow for Neon Blocks
    constexpr uint32_t PURPLE      = 0xA78BFA;  // Purple accent
    constexpr uint32_t GOLD        = 0xF59E0B;  // Gold/amber
}

// ========================================
// EXTENDED HELPERS FOR CLOCK FACES
// ========================================

/**
 * Create a colored square icon placeholder
 *
 * @param parent Parent object
 * @param size Square size in pixels
 * @param color Fill color
 * @param x X position
 * @param y Y position
 * @return Pointer to created object
 */
inline lv_obj_t* createIconPlaceholder(lv_obj_t* parent, int size, uint32_t color,
                                      lv_align_t align = LV_ALIGN_CENTER,
                                      int x_offset = 0, int y_offset = 0) {
    lv_obj_t* icon = lv_obj_create(parent);
    lv_obj_set_size(icon, size, size);
    lv_obj_set_style_radius(icon, 4, 0);  // Slightly rounded corners
    lv_obj_set_style_bg_color(icon, lv_color_hex(color), 0);
    lv_obj_set_style_border_width(icon, 0, 0);
    lv_obj_clear_flag(icon, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_align(icon, align, x_offset, y_offset);
    return icon;
}

/**
 * Create an arc gauge (circular progress indicator)
 *
 * @param parent Parent object
 * @param size Diameter in pixels
 * @param bg_color Background arc color
 * @param fg_color Progress arc color
 * @param width Arc line width
 * @param start_angle Start angle in degrees (0 = top)
 * @param end_angle End angle in degrees
 * @return Pointer to created arc
 */
inline lv_obj_t* createArcGauge(lv_obj_t* parent, int size,
                               uint32_t bg_color, uint32_t fg_color,
                               int width = 12,
                               int start_angle = 135, int end_angle = 45) {
    lv_obj_t* arc = lv_arc_create(parent);
    lv_obj_set_size(arc, size, size);
    lv_obj_center(arc);

    // Background arc
    lv_obj_set_style_arc_width(arc, width, LV_PART_MAIN);
    lv_obj_set_style_arc_color(arc, lv_color_hex(bg_color), LV_PART_MAIN);

    // Progress arc (indicator)
    lv_obj_set_style_arc_width(arc, width, LV_PART_INDICATOR);
    lv_obj_set_style_arc_color(arc, lv_color_hex(fg_color), LV_PART_INDICATOR);

    // Remove knob (display-only)
    lv_obj_remove_style(arc, NULL, LV_PART_KNOB);
    lv_obj_clear_flag(arc, LV_OBJ_FLAG_CLICKABLE);

    // Set arc angles
    lv_arc_set_bg_angles(arc, start_angle, end_angle);
    lv_arc_set_rotation(arc, 0);
    lv_arc_set_range(arc, 0, 100);
    lv_arc_set_value(arc, 0);

    return arc;
}

/**
 * Create a card container with rounded corners
 *
 * @param parent Parent object
 * @param width Card width
 * @param height Card height
 * @param bg_color Background color
 * @param radius Corner radius
 * @return Pointer to created card
 */
inline lv_obj_t* createCard(lv_obj_t* parent, int width, int height,
                            uint32_t bg_color = 0x1e293b, int radius = 16) {
    lv_obj_t* card = lv_obj_create(parent);
    lv_obj_set_size(card, width, height);
    lv_obj_set_style_radius(card, radius, 0);
    lv_obj_set_style_bg_color(card, lv_color_hex(bg_color), 0);
    lv_obj_set_style_bg_opa(card, LV_OPA_90, 0);
    lv_obj_set_style_border_width(card, 2, 0);
    lv_obj_set_style_border_color(card, lv_color_hex(0x475569), 0);
    lv_obj_set_style_border_opa(card, LV_OPA_50, 0);
    lv_obj_set_style_pad_all(card, 12, 0);
    lv_obj_clear_flag(card, LV_OBJ_FLAG_SCROLLABLE);
    return card;
}

/**
 * Create a colored block (for Neon Blocks design)
 *
 * @param parent Parent object
 * @param width Block width
 * @param height Block height
 * @param color Block color
 * @param align Alignment
 * @param x_offset X offset
 * @param y_offset Y offset
 * @return Pointer to created block
 */
inline lv_obj_t* createColorBlock(lv_obj_t* parent, int width, int height,
                                  uint32_t color, lv_align_t align,
                                  int x_offset, int y_offset) {
    lv_obj_t* block = lv_obj_create(parent);
    lv_obj_set_size(block, width, height);
    lv_obj_set_style_radius(block, 8, 0);
    lv_obj_set_style_bg_color(block, lv_color_hex(color), 0);
    lv_obj_set_style_border_width(block, 0, 0);
    lv_obj_set_style_pad_all(block, 0, 0);
    lv_obj_clear_flag(block, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_align(block, align, x_offset, y_offset);
    return block;
}

/**
 * Create a vertical activity bar (for Fitness Pro design)
 *
 * @param parent Parent object
 * @param width Bar width
 * @param max_height Maximum bar height
 * @param color Bar color
 * @param align Alignment
 * @param x_offset X offset
 * @param y_offset Y offset from bottom
 * @return Pointer to created bar
 */
inline lv_obj_t* createVerticalBar(lv_obj_t* parent, int width, int max_height,
                                   uint32_t color, lv_align_t align,
                                   int x_offset, int y_offset) {
    lv_obj_t* bar = lv_obj_create(parent);
    lv_obj_set_size(bar, width, max_height);
    lv_obj_set_style_radius(bar, width / 2, 0);  // Rounded ends
    lv_obj_set_style_bg_color(bar, lv_color_hex(color), 0);
    lv_obj_set_style_border_width(bar, 0, 0);
    lv_obj_clear_flag(bar, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_align(bar, align, x_offset, y_offset);
    return bar;
}

/**
 * Set vertical gradient background
 *
 * @param obj Object to apply gradient to
 * @param top_color Top color
 * @param bottom_color Bottom color
 */
inline void setGradientBackground(lv_obj_t* obj, uint32_t top_color, uint32_t bottom_color) {
    lv_obj_set_style_bg_color(obj, lv_color_hex(top_color), 0);
    lv_obj_set_style_bg_grad_color(obj, lv_color_hex(bottom_color), 0);
    lv_obj_set_style_bg_grad_dir(obj, LV_GRAD_DIR_VER, 0);
    lv_obj_set_style_bg_opa(obj, LV_OPA_COVER, 0);
}

/**
 * Add text shadow for better contrast
 *
 * @param obj Object to add shadow to
 * @param shadow_width Shadow blur width
 * @param shadow_ofs_y Shadow Y offset
 */
inline void addTextShadow(lv_obj_t* obj, int shadow_width = 10, int shadow_ofs_y = 2) {
    lv_obj_set_style_shadow_width(obj, shadow_width, 0);
    lv_obj_set_style_shadow_color(obj, lv_color_hex(0x000000), 0);
    lv_obj_set_style_shadow_opa(obj, LV_OPA_60, 0);
    lv_obj_set_style_shadow_ofs_y(obj, shadow_ofs_y, 0);
}

} // namespace Doki

#endif // DOKI_LVGL_HELPERS_H
