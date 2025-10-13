/**
 * LVGL Configuration for ESP32-S3 Triple Display System
 * Phase 1: Single display setup with PSRAM optimization
 */

#ifndef LV_CONF_H
#define LV_CONF_H

#include <stdint.h>

/*====================
   COLOR SETTINGS
 *====================*/

/* Color depth: 1 (1 byte per pixel), 8 (RGB332), 16 (RGB565), 32 (ARGB8888) */
#define LV_COLOR_DEPTH 16

/* Swap the 2 bytes of RGB565 color. Useful if the display has an 8-bit interface (e.g. SPI) */
#define LV_COLOR_16_SWAP 0

/* Enable more complex drawing routines to manage screens transparency */
#define LV_COLOR_SCREEN_TRANSP 0

/* Adjust color mix functions rounding. GPUs might calculate color mix differently */
#define LV_COLOR_MIX_ROUND_OFS 0

/*=========================
   MEMORY SETTINGS
 *=========================*/

/* Use standard memory allocation (we'll manually use PSRAM for buffers) */
#define LV_MEM_CUSTOM 0
#if LV_MEM_CUSTOM == 1
    #define LV_MEM_CUSTOM_INCLUDE <stdlib.h>
    #define LV_MEM_CUSTOM_ALLOC   malloc
    #define LV_MEM_CUSTOM_FREE    free
    #define LV_MEM_CUSTOM_REALLOC realloc
#else
    #define LV_MEM_SIZE (64U * 1024U)  /* 64KB for LVGL internal memory */
    #define LV_MEM_ADR 0              /* Address for memory pool (not used) */
    #define LV_MEM_AUTO_DEFRAG 1      /* Automatically defragment on free */
#endif

/* Number of memory buffer sizes */
#define LV_MEM_BUF_MAX_NUM 16

/*====================
   HAL SETTINGS
 *====================*/

/* Default display refresh period in milliseconds */
#define LV_DISP_DEF_REFR_PERIOD 30

/* Input device read period in milliseconds */
#define LV_INDEV_DEF_READ_PERIOD 30

/* Use a custom tick source for LVGL */
#define LV_TICK_CUSTOM 1
#if LV_TICK_CUSTOM
    #define LV_TICK_CUSTOM_INCLUDE "Arduino.h"
    #define LV_TICK_CUSTOM_SYS_TIME_EXPR (millis())
#endif

/*=================
   FEATURE USAGE
 *=================*/

/* Enable the log module */
#define LV_USE_LOG 1
#if LV_USE_LOG
    #define LV_LOG_LEVEL LV_LOG_LEVEL_WARN
    #define LV_LOG_PRINTF 1
#endif

/* Enable assertions for debugging */
#define LV_USE_ASSERT_NULL          1
#define LV_USE_ASSERT_MALLOC        1
#define LV_USE_ASSERT_STYLE         0
#define LV_USE_ASSERT_MEM_INTEGRITY 0
#define LV_USE_ASSERT_OBJ           0

/*==================
   DRAWING SETTINGS
 *==================*/

/* Enable complex draw engine (for shadows, gradients, etc.) */
#define LV_DRAW_COMPLEX 1

/* Default image cache size. 0 to disable caching */
#define LV_IMG_CACHE_DEF_SIZE 0

/* Maximum buffer size to allocate for rotation */
#define LV_DISP_ROT_MAX_BUF (10*1024)

/*=================
   GPU SETTINGS
 *=================*/

/* Use ESP32's DMA for faster display updates */
#define LV_USE_GPU_ESP32_DMA 0  /* Set to 1 if you want to use DMA */

/*==================
   FONT SETTINGS
 *==================*/

/* Montserrat fonts with various sizes */
#define LV_FONT_MONTSERRAT_8  0
#define LV_FONT_MONTSERRAT_10 1
#define LV_FONT_MONTSERRAT_12 1
#define LV_FONT_MONTSERRAT_14 1
#define LV_FONT_MONTSERRAT_16 1
#define LV_FONT_MONTSERRAT_18 1
#define LV_FONT_MONTSERRAT_20 1
#define LV_FONT_MONTSERRAT_22 0
#define LV_FONT_MONTSERRAT_24 1
#define LV_FONT_MONTSERRAT_26 0
#define LV_FONT_MONTSERRAT_28 0
#define LV_FONT_MONTSERRAT_30 0
#define LV_FONT_MONTSERRAT_32 1
#define LV_FONT_MONTSERRAT_34 0
#define LV_FONT_MONTSERRAT_36 0
#define LV_FONT_MONTSERRAT_38 0
#define LV_FONT_MONTSERRAT_40 0
#define LV_FONT_MONTSERRAT_42 0
#define LV_FONT_MONTSERRAT_44 0
#define LV_FONT_MONTSERRAT_46 0
#define LV_FONT_MONTSERRAT_48 1

/* Default font */
#define LV_FONT_DEFAULT &lv_font_montserrat_14

/*==================
   WIDGET SETTINGS
 *==================*/

/* Enable all widgets (we'll optimize later) */
#define LV_USE_ARC        1
#define LV_USE_BAR        1
#define LV_USE_BTN        1
#define LV_USE_BTNMATRIX  1
#define LV_USE_CANVAS     1
#define LV_USE_CHECKBOX   1
#define LV_USE_DROPDOWN   1
#define LV_USE_IMG        1
#define LV_USE_LABEL      1
#define LV_USE_LINE       1
#define LV_USE_ROLLER     1
#define LV_USE_SLIDER     1
#define LV_USE_SWITCH     1
#define LV_USE_TEXTAREA   1
#define LV_USE_TABLE      1

/*==================
   THEME SETTINGS
 *==================*/

#define LV_USE_THEME_DEFAULT 1
#if LV_USE_THEME_DEFAULT
    #define LV_THEME_DEFAULT_DARK 1
    #define LV_THEME_DEFAULT_GROW 1
    #define LV_THEME_DEFAULT_TRANSITION_TIME 80
#endif

/*==================
   LAYOUT SETTINGS
 *==================*/

#define LV_USE_FLEX  1
#define LV_USE_GRID  1

/*===================
   ANIMATION SETTINGS
 *===================*/

#define LV_USE_ANIMATION 1

/*==================
   OTHERS
 *==================*/

#define LV_USE_SPRINTF_CUSTOM 0

/* Enable filesystem support (for later: icons, images) */
#define LV_USE_FS_STDIO 0
#define LV_USE_FS_POSIX 0
#define LV_USE_FS_WIN32 0
#define LV_USE_FS_FATFS 0

/* Enable PNG decoder (useful for icons later) */
#define LV_USE_PNG 0

/* Enable BMP decoder */
#define LV_USE_BMP 0

/* Enable JPG decoder */
#define LV_USE_SJPG 0

/* Enable GIF decoder */
#define LV_USE_GIF 0

/* Enable QR code generation */
#define LV_USE_QRCODE 0

/*==================
   EXAMPLES
 *==================*/

#define LV_BUILD_EXAMPLES 0

/*==================
   DEMO
 *==================*/

#define LV_USE_DEMO_WIDGETS 0
#define LV_USE_DEMO_KEYPAD_AND_ENCODER 0
#define LV_USE_DEMO_BENCHMARK 0
#define LV_USE_DEMO_STRESS 0
#define LV_USE_DEMO_MUSIC 0

#endif /* LV_CONF_H */