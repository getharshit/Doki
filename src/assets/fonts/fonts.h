/**
 * Custom Fonts for Doki OS Clock Faces
 *
 * This file declares all custom fonts used in the smartwatch-style clock designs.
 * All fonts have been converted to LVGL format using lv_font_conv.
 *
 * Usage:
 *   #include "assets/fonts/fonts.h"
 *   lv_obj_set_style_text_font(label, &ka1_80, 0);
 */

#ifndef CUSTOM_FONTS_H
#define CUSTOM_FONTS_H

#include <lvgl.h>

// ========================================
// NEON BLOCKS FONTS (ka1 font family)
// ========================================
// Geometric, blocky style for modular clock face
LV_FONT_DECLARE(ka1_80);    // Large time display (hours:minutes)
LV_FONT_DECLARE(ka1_48);    // Counter/bottom display
LV_FONT_DECLARE(ka1_36);    // Info numbers (battery %, temp, BPM)
LV_FONT_DECLARE(ka1_20);    // Labels (PM, MAR, BPM)

// ========================================
// CYBER MATRIX FONTS (PixelOperator)
// ========================================
// Retro dot matrix LED style
LV_FONT_DECLARE(pixel_operator_72);  // Large time display (increased from 56)
LV_FONT_DECLARE(pixel_operator_56);  // Medium time display
LV_FONT_DECLARE(pixel_operator_18);  // Date text (MON, JUL 28)
LV_FONT_DECLARE(pixel_operator_14);  // Small labels and stats

// ========================================
// RAINBOW WAVE FONTS (Inter)
// ========================================
// Modern Apple Watch style with gradient background
LV_FONT_DECLARE(inter_bold_84);     // Giant time display
LV_FONT_DECLARE(inter_medium_20);   // Date text
LV_FONT_DECLARE(inter_medium_18);   // Weather temp

// ========================================
// FITNESS PRO FONTS (Inter)
// ========================================
// Clean modern style with activity bars
LV_FONT_DECLARE(inter_black_96);    // Huge time display
LV_FONT_DECLARE(inter_semibold_28); // AM/PM indicator
LV_FONT_DECLARE(inter_black_16);    // Stats numbers and labels
LV_FONT_DECLARE(inter_black_12);    // Small text

// ========================================
// ZEN GLOW FONTS (BallsoOnTheRampage)
// ========================================
// Minimal dot style with gradient glow
LV_FONT_DECLARE(ballso_96);  // Large dot-style time (increased from 72)
LV_FONT_DECLARE(ballso_72);  // Medium dot-style time
// Note: Uses inter_medium_20 and inter_medium_18 from Rainbow Wave for date text

#endif // CUSTOM_FONTS_H
