#include "sdcard.h"
#include <stdbool.h>
#include <errno.h>
#include <stddef.h>
#include <zephyr/fs/fs.h>
#include <zephyr/sys/printk.h>
#include <zephyr/storage/disk_access.h>
#include <ff.h>
#include <string.h>


// FatFs Setup
static const char *pdrv = "SD";
static const char *test_path = "/SD:/TEST.TXT";
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

    /* --------- File write test --------- */
    struct fs_file_t f;
    fs_file_t_init(&f);

    ret = fs_unlink(test_path);
    if (!((ret == 0) || (ret == -ENOENT))) {
        printk("SD Card: unlink failed (%d)\n", ret);
        return ret;
    }

    ret = fs_open(&f, test_path, (FS_O_WRITE | FS_O_CREATE));
    if (ret != 0) {
        printk("SD Card: failed to open file for write (%d)\n", ret);
        return ret;
    }

    const char *test_line = "SD CARD WRITE TEST";
    const size_t test_line_len = strlen(test_line);
    ssize_t n = fs_write(&f, test_line, test_line_len);
    ret = fs_close(&f);
    if ((n < 0) || ((size_t)n != test_line_len)) {
        if (n < 0) {
            printk("SD Card: write failed (%zd)\n", n);
            return n;
        }
        printk("SD Card: short write, (%zd) of (%zu)\n", n, test_line_len);
        return -EIO;
    }
    if (ret != 0) {
        printk("SD Card: failed to close file for write (%d)\n", ret);
        return ret;
    }

    /* --------- File read test --------- */
    fs_file_t_init(&f);
    char read_buf[32];
    ret = fs_open(&f, test_path, FS_O_READ);
    if (ret != 0) {
        printk("SD Card: failed to open file for read (%d)\n", ret);
        return ret;
    }

    n = fs_read(&f, read_buf, sizeof(read_buf));
    ret = fs_close(&f);
    if (n < 0) {
        printk("SD Card: failed to read test line (%zd)\n", n);
        return n;
    }
    if ((size_t)n != test_line_len) {
        printk("SD Card: wrong line length, (%zd) of (%zu)\n", n, test_line_len);
        return -EIO;
    }
    if (memcmp(read_buf, test_line, test_line_len) != 0) {
        printk("SD Card: content mismatch (%.*s)\n", (int)test_line_len, read_buf);
        return -EIO;
    }
    if (ret != 0) {
        printk("SD Card: failed to close file for read (%d)\n", ret);
        return ret;
    }

    sdcard_flag = true;
    return 0;
}