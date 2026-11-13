#include "sdcard.h"
#include <stdbool.h>
#include <zephyr/fs/fs.h>
#include <zephyr/sys/printk.h>
#include <zephyr/storage/disk_access.h>
#include <ff.h>

static const char *pdrv = "SD";
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

int sdcard_init(void) {
    int ret = disk_access_init(pdrv);
    if (ret != 0) {
        printk("SD Card: disk access failed (%d)\n", ret);
        return ret;
    }

    ret = fs_mount(&sdcard_mount);
    if (ret != 0) {
        printk("SD Card: mount failed (%d)\n", ret);
        return ret;
    }

    sdcard_flag = true;
    return 0;
}