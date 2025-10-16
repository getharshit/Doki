/**
 * @file lvgl_mem.h
 * @brief PSRAM memory allocation wrappers for LVGL
 *
 * Provides custom memory allocation functions that use ESP32-S3's
 * PSRAM for LVGL's internal memory, including image decoding buffers.
 */

#ifndef LVGL_MEM_H
#define LVGL_MEM_H

#include <Arduino.h>
#include <esp_heap_caps.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Allocate memory from PSRAM for LVGL
 * @param size Size in bytes to allocate
 * @return Pointer to allocated memory, or NULL on failure
 */
static inline void* lv_psram_malloc(size_t size) {
    void* ptr = heap_caps_malloc(size, MALLOC_CAP_SPIRAM);
    if (ptr == NULL) {
        // Fallback to regular heap if PSRAM is full
        ptr = malloc(size);
    }
    return ptr;
}

/**
 * @brief Reallocate memory from PSRAM for LVGL
 * @param ptr Pointer to existing memory
 * @param size New size in bytes
 * @return Pointer to reallocated memory, or NULL on failure
 */
static inline void* lv_psram_realloc(void* ptr, size_t size) {
    void* new_ptr = heap_caps_realloc(ptr, size, MALLOC_CAP_SPIRAM);
    if (new_ptr == NULL && size > 0) {
        // Fallback to regular heap if PSRAM is full
        new_ptr = realloc(ptr, size);
    }
    return new_ptr;
}

#ifdef __cplusplus
}
#endif

#endif // LVGL_MEM_H
