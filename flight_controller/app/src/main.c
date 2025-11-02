#include <zephyr/kernel.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/sys/printk.h>

/* ======= Synchronization Objects ======= */
K_SEM_DEFINE(lidar_sem, 0, 1);

struct lidar_msg {
    uint16_t distance_mm;
};
K_MSGQ_DEFINE(lidar_msgq, sizeof(struct lidar_msg), 8, 4);

/* ======= Timer ISR ======= */
void lidar_timer_handler(struct k_timer *timer_id)
{
    k_sem_give(&lidar_sem);
}
K_TIMER_DEFINE(lidar_timer, lidar_timer_handler, NULL);

/* ======= LiDAR Read Thread ======= */
void lidar_thread(void)
{
    const struct device *dev = DEVICE_DT_GET_ONE(grmn_lidar_lite_v4);
    struct sensor_value val;
    struct lidar_msg msg;

    if (!device_is_ready(dev)) {
        printk("LIDAR device not ready!\n");
        return;
    }

    while (1) {
        /* Wait until ISR releases semaphore */
        k_sem_take(&lidar_sem, K_FOREVER);

        if (sensor_sample_fetch(dev) == 0 &&
            sensor_channel_get(dev, SENSOR_CHAN_DISTANCE, &val) == 0) {
            msg.distance_mm = val.val1 * 1000 + val.val2 / 1000;
            k_msgq_put(&lidar_msgq, &msg, K_NO_WAIT);
        }
    }
}
K_THREAD_DEFINE(lidar_tid, 2048, lidar_thread, NULL, NULL, NULL,
                7, 0, 0);

/* ======= Main Thread ======= */
void main(void)
{
    struct lidar_msg msg;

    printk("Starting LiDAR-Lite V4 test app...\n");

    /* Timer period = 50 ms ⇒ 20 Hz polling rate */
    k_timer_start(&lidar_timer, K_MSEC(50), K_MSEC(50));

    while (1) {
        printk("test to see if we get msg\n");
        if (k_msgq_get(&lidar_msgq, &msg, K_FOREVER) == 0) {
            printk("Distance: %u mm\n", msg.distance_mm);

        }
    }
}