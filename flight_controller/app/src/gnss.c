#include "gnss.h"
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/uart.h>
#include <string.h>

static const struct device *gnss_dev = NULL;

static char line[GNSS_NMEA_MAX_LEN];
static size_t line_len = 0;

void gnss_timer_handler(struct k_timer *timer_id) {
    k_sem_give(&gnss_sem);
}

static int process_sam_m8q(const struct device *dev, struct gnss_sentence *out) {
    unsigned char c;

    while (uart_poll_in(dev, &c) == 0) {
        if (c == '$') {
            line_len = 0;
        } else if (line_len == 0 || c == '\r') {
            continue;
        }

        if (c == '\n') {
            line[line_len] = '\0';
            memcpy(out->text, line, line_len + 1);
            out->time = k_uptime_get_32();
            line_len = 0;
            return 0;
        }
        if (line_len < GNSS_NMEA_MAX_LEN - 1) {
            line[line_len++] = c;
        }
    }
    return -EAGAIN;
}

void gnss_read_thread(void *arg1, void *arg2, void *arg3) {
    while(1) {
        k_sem_take(&gnss_sem, K_FOREVER);
        if (gnss_dev == NULL) {
            continue;
        }

        struct gnss_sentence data;
        while (process_sam_m8q(gnss_dev, &data) == 0) {
            int put_rc = k_msgq_put(&gnss_msgq, &data, K_NO_WAIT);
            if (put_rc != 0) {
                printk("GNSS msgq full, dropping data\n");
            }
        }
    }
}

int gnss_init(const struct device *dev) {
    if(dev == NULL) {
        printk("GNSS device not found\n");
        return -1;
    }
    if(!device_is_ready(dev)) {
        printk("GNSS device not ready\n");
        return -1;
    }
    gnss_dev = dev;
    printk("GNSS initialized successfully\n");
    return 0;
}

int gnss_get(struct gnss_sentence *out, k_timeout_t timeout) {
    if(out == NULL) {
        printk("gnss_get: output pointer is NULL\n");
        return -1;
    }
    return k_msgq_get(&gnss_msgq, out, timeout);
}
