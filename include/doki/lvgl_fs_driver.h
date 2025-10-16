/**
 * @file lvgl_fs_driver.h
 * @brief LVGL SPIFFS Filesystem Driver
 *
 * Registers SPIFFS with LVGL so images/GIFs can be loaded directly
 * from filesystem using paths like "S:/media/image.png"
 */

#ifndef LVGL_FS_DRIVER_H
#define LVGL_FS_DRIVER_H

#include <lvgl.h>
#include "FS.h"
#include "SPIFFS.h"

namespace Doki {

/**
 * @brief LVGL Filesystem Driver for SPIFFS
 *
 * This class registers SPIFFS with LVGL's filesystem abstraction,
 * allowing LVGL to load images and GIFs directly from SPIFFS.
 */
class LvglFsDriver {
public:
    /**
     * @brief Initialize and register LVGL filesystem driver
     * Must be called after SPIFFS is mounted and LVGL is initialized
     * @return true if successful, false otherwise
     */
    static bool init();

private:
    // LVGL filesystem callbacks
    static void* fs_open(lv_fs_drv_t* drv, const char* path, lv_fs_mode_t mode);
    static lv_fs_res_t fs_close(lv_fs_drv_t* drv, void* file_p);
    static lv_fs_res_t fs_read(lv_fs_drv_t* drv, void* file_p, void* buf, uint32_t btr, uint32_t* br);
    static lv_fs_res_t fs_seek(lv_fs_drv_t* drv, void* file_p, uint32_t pos, lv_fs_whence_t whence);
    static lv_fs_res_t fs_tell(lv_fs_drv_t* drv, void* file_p, uint32_t* pos);

    static lv_fs_drv_t _fs_drv;
};

} // namespace Doki

#endif // LVGL_FS_DRIVER_H
