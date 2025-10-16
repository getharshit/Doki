/**
 * @file lvgl_fs_driver.cpp
 * @brief Implementation of LVGL SPIFFS filesystem driver
 */

#include "doki/lvgl_fs_driver.h"

namespace Doki {

// Static member initialization
lv_fs_drv_t LvglFsDriver::_fs_drv;

bool LvglFsDriver::init() {
    Serial.println("[LvglFsDriver] Initializing LVGL filesystem driver...");

    // Initialize filesystem driver
    lv_fs_drv_init(&_fs_drv);

    // Set the letter for this driver (S: for SPIFFS)
    _fs_drv.letter = 'S';

    // Set callbacks
    _fs_drv.open_cb = fs_open;
    _fs_drv.close_cb = fs_close;
    _fs_drv.read_cb = fs_read;
    _fs_drv.seek_cb = fs_seek;
    _fs_drv.tell_cb = fs_tell;

    // Register the driver
    lv_fs_drv_register(&_fs_drv);

    Serial.println("[LvglFsDriver] ✓ Registered successfully (use 'S:' prefix for SPIFFS paths)");
    return true;
}

void* LvglFsDriver::fs_open(lv_fs_drv_t* drv, const char* path, lv_fs_mode_t mode) {
    (void)drv; // Unused parameter

    // Remove leading slash if present
    const char* filepath = path;
    if (filepath[0] == '/') {
        filepath = path;
    } else {
        // Add leading slash for SPIFFS
        static char fullpath[256];
        snprintf(fullpath, sizeof(fullpath), "/%s", path);
        filepath = fullpath;
    }

    // Open file based on mode
    File* file = new File();
    if (mode == LV_FS_MODE_RD) {
        *file = SPIFFS.open(filepath, "r");
    } else if (mode == LV_FS_MODE_WR) {
        *file = SPIFFS.open(filepath, "w");
    } else if (mode == LV_FS_MODE_WR | LV_FS_MODE_RD) {
        *file = SPIFFS.open(filepath, "r+");
    }

    if (!*file) {
        Serial.printf("[LvglFsDriver] Error: Failed to open file: %s\n", filepath);
        delete file;
        return nullptr;
    }

    Serial.printf("[LvglFsDriver] Opened file: %s\n", filepath);
    return (void*)file;
}

lv_fs_res_t LvglFsDriver::fs_close(lv_fs_drv_t* drv, void* file_p) {
    (void)drv; // Unused parameter

    if (file_p == nullptr) {
        return LV_FS_RES_INV_PARAM;
    }

    File* file = (File*)file_p;
    file->close();
    delete file;

    return LV_FS_RES_OK;
}

lv_fs_res_t LvglFsDriver::fs_read(lv_fs_drv_t* drv, void* file_p, void* buf, uint32_t btr, uint32_t* br) {
    (void)drv; // Unused parameter

    if (file_p == nullptr || buf == nullptr || br == nullptr) {
        return LV_FS_RES_INV_PARAM;
    }

    File* file = (File*)file_p;
    *br = file->read((uint8_t*)buf, btr);

    return (*br == btr || file->available() == 0) ? LV_FS_RES_OK : LV_FS_RES_UNKNOWN;
}

lv_fs_res_t LvglFsDriver::fs_seek(lv_fs_drv_t* drv, void* file_p, uint32_t pos, lv_fs_whence_t whence) {
    (void)drv; // Unused parameter

    if (file_p == nullptr) {
        return LV_FS_RES_INV_PARAM;
    }

    File* file = (File*)file_p;

    SeekMode mode;
    switch (whence) {
        case LV_FS_SEEK_SET:
            mode = SeekSet;
            break;
        case LV_FS_SEEK_CUR:
            mode = SeekCur;
            break;
        case LV_FS_SEEK_END:
            mode = SeekEnd;
            break;
        default:
            return LV_FS_RES_INV_PARAM;
    }

    if (!file->seek(pos, mode)) {
        return LV_FS_RES_UNKNOWN;
    }

    return LV_FS_RES_OK;
}

lv_fs_res_t LvglFsDriver::fs_tell(lv_fs_drv_t* drv, void* file_p, uint32_t* pos) {
    (void)drv; // Unused parameter

    if (file_p == nullptr || pos == nullptr) {
        return LV_FS_RES_INV_PARAM;
    }

    File* file = (File*)file_p;
    *pos = file->position();

    return LV_FS_RES_OK;
}

} // namespace Doki
