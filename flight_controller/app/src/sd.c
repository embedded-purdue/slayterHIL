#include "sd.h"
#include <zephyr/fs/fs.h>
#include <zephyr/sys/printk.h>
#include <ff.h>

static bool sd_flag;
static FATFS fat_fs;
static struct fs_mount_t mount_point = {
    .type = FS_FATFS,
    .fs_data = &fat_fs,
    .flags = FS_MOUNT_FLAG_USE_DISK_ACCESS
};

bool sd_ready(void) {
    return sd_flag;
}