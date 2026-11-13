#include "sdcard.h"
#include <stdbool.h>
#include <zephyr/fs/fs.h>
#include <zephyr/sys/printk.h>
#include <ff.h>

static bool sdcard_flag = false;
static FATFS fat_fs;
static struct fs_mount_t sdcard_mount = {
    .type = FS_FATFS,
    .mnt_point = "/SD:",
    .fs_data = &fat_fs,
    .flags = (FS_MOUNT_FLAG_USE_DISK_ACCESS | FS_MOUNT_FLAG_NO_FORMAT)
};

bool sdcard_ready(void) {
    return sdcard_flag;
}