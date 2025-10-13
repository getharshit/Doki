/**
 * ST7789 Display Driver Implementation
 * Place in: src/st7789_driver.cpp
 */

#include "st7789_driver.h"

// Global driver instance
ST7789_Driver display_driver;

/**
 * LVGL Flush Callback
 * Called by LVGL when it needs to update the display
 */
void lvgl_flush_cb(lv_disp_drv_t* disp, const lv_area_t* area, lv_color_t* color_p) {
    uint32_t w = (area->x2 - area->x1 + 1);
    uint32_t h = (area->y2 - area->y1 + 1);
    
    // Set the drawing window
    display_driver.setAddrWindow(area->x1, area->y1, area->x2, area->y2);
    
    // Push the pixel data to the display
    display_driver.pushColors((uint16_t*)&color_p->full, w * h);
    
    // Tell LVGL we're done flushing
    lv_disp_flush_ready(disp);
}