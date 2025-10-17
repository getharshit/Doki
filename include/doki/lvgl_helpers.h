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
}

} // namespace Doki

#endif // DOKI_LVGL_HELPERS_H
