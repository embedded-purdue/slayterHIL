#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h> 
#include <zephyr/drivers/sensor.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/i2c.h>
#include "state_machine.h"
// #include "imu.h"
#include "lidar.h"
#include "bno.h"
#include "ledCtrl.h"
#include "uart.h"
#include <stdio.h> 
#include "pid.h"
#include "control.h"
#include <stdalign.h>


// static const struct device *bno055_dev = DEVICE_DT_GET(DT_NODELABEL(bno)); 

// define thread
#define STACK_SIZE      2048
#define COMMS_PRIORITY  1
#define BNO_STACK_SIZE 4096
#define BNO_PRIORITY   3
#define LIDAR_STACK_SIZE 2048
#define LIDAR_PRIORITY 4
#define MAX_BNO_MSGQ_LEN 16
#define MAX_LIDAR_MSGQ_LEN 8
#define BNO_SEM_INIT_COUNT 0 
#define BNO_SEM_MAX_COUNT 1
#define BNO_THREAD_PRIORITY 2
#define UART_CONSUMER_STACK_SIZE 2048
#define UART_CONSUMER_PRIORITY 5
#define TOP_LEFT  1
#define TOP_RIGHT  2
#define BOTTOM_LEFT  0
#define BOTTOM_RIGHT  3

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

//lidar section
K_SEM_DEFINE(lidar_sem, 0, 1);
K_MSGQ_DEFINE(lidar_msgq, sizeof(struct lidar_data), MAX_LIDAR_MSGQ_LEN, alignof(struct lidar_data));
K_TIMER_DEFINE(lidar_timer, lidar_timer_handler, NULL);
//LiDAR stack definiton 
K_THREAD_STACK_DEFINE(lidar_consumer_stack, LIDAR_STACK_SIZE); 
static struct k_thread lidar_consumer_thread_data;

//imu section
K_THREAD_STACK_DEFINE(state_machine_stack, STACK_SIZE);
static struct k_thread state_machine_thread_data;

K_THREAD_STACK_DEFINE(bno_consumer_stack, BNO_STACK_SIZE);
static struct k_thread bno_consumer_thread_data;


K_MSGQ_DEFINE(bno_msgq, sizeof(struct bno_data), MAX_BNO_MSGQ_LEN, alignof(int));
K_SEM_DEFINE(bno_sem, BNO_SEM_INIT_COUNT, BNO_SEM_MAX_COUNT);


K_THREAD_DEFINE(bno_thread, BNO_STACK_SIZE, bno_read_thread, NULL, NULL, NULL, BNO_THREAD_PRIORITY, 0, 0);
K_THREAD_DEFINE(lidar_thread, LIDAR_STACK_SIZE, lidar_read_thread, NULL, NULL, NULL, LIDAR_PRIORITY, 0, 0);

//uart section
K_THREAD_STACK_DEFINE(uart_consumer_stack, UART_CONSUMER_STACK_SIZE);
static struct k_thread uart_consumer_thread_data;
K_MSGQ_DEFINE(uart_rx_msgq, sizeof(struct uart_msg), 16, alignof(struct uart_msg));

static const struct gpio_dt_spec led0 = GPIO_DT_SPEC_GET(DT_ALIAS(led4), gpios);
/* Running count of bytes accepted from uart1 — shown in telemetry so you can
 * see at a glance whether commands physically reach the RX pin. */
static volatile uint32_t uart_rx_count = 0;
/* Split a float into sign + integer + hundredths so telemetry prints fixed
 * point with plain %d and skips the stack-heavy floating-point printk path. */
static void fx2(float v, char *sign, int *whole, int *frac)
{
    *sign  = (v < 0.0f) ? '-' : ' ';
    float a = (v < 0.0f) ? -v : v;
    *whole = (int)a;
    *frac  = (int)((a - (float)*whole) * 100.0f + 0.5f);
    if (*frac >= 100) { *frac -= 100; (*whole)++; }
}

static void bno_consumer(void *arg1, void *arg2, void *arg3)
{
    struct bno_data out;
    pid_ctrl_t pitch_pid, roll_pid, alt_pid;

    printk("BNO consumer thread started\n");

    /*
     * Attitude gains — tune Kp first, then Ki to kill steady-state offset,
     * then Kd to reduce overshoot. integral_limit caps the I term to ±20
     * output units. Starting points from prior proportional-only tuning:
     *   pitch ±3.5° observed range, roll ±1.5° observed range.
     */
    pid_init(&pitch_pid, /*kp=*/8.0f,  /*ki=*/0.5f,   /*kd=*/2.0f,  /*lim=*/20.0f);
    pid_init(&roll_pid,  /*kp=*/10.0f, /*ki=*/0.5f,   /*kd=*/2.0f,  /*lim=*/20.0f);
    /*
     * Altitude gains operate on error in millimetres, so they are much
     * smaller. Output is a collective throttle offset added to every motor.
     * integral_limit caps collective authority from the I term to ±30.
     */
    pid_init(&alt_pid,   /*kp=*/0.05f, /*ki=*/0.001f, /*kd=*/0.02f, /*lim=*/30.0f);

    uint32_t last_time = k_uptime_get_32();
    int print_counter = 0;

    while (1) {
        /* Block for one sample, then drain any backlog and keep only the
         * newest: the producer runs faster (25 Hz) than this loop (20 Hz),
         * so this both prevents queue-full drops and guarantees the control
         * loop acts on the freshest attitude, never a stale FIFO entry. */
        int rc = bno_get(&out, K_MSEC(100));
        if (rc == 0) {
            struct bno_data newer;
            while (bno_get(&newer, K_NO_WAIT) == 0) {
                out = newer;
            }
        }
        if (rc != 0) {
            /* Sensor loss — idle motors and reset PID state to prevent
             * stale integral and derivative carry-over on recovery. */
            pid_reset(&pitch_pid);
            pid_reset(&roll_pid);
            pid_reset(&alt_pid);
            last_time = k_uptime_get_32();
            for (int i = 0; i < 4; i++) {
                set_led_intensity(i, 10);
            }
            k_msleep(50);
            continue;
        }

        uint32_t now = k_uptime_get_32();
        float dt = (float)(now - last_time) / 1000.0f;  /* ms -> seconds */
        last_time = now;

        /* Guard: skip cycle if dt is zero or suspiciously large (scheduler
         * stall, wrap-around, first tick). */
        if (dt <= 0.0f || dt > 0.5f) {
            k_msleep(50);
            continue;
        }

        /* Snapshot RC-commanded setpoints + latest altitude for this tick. */
        float pitch_sp, roll_sp, altitude_sp;
        uint32_t altitude_mm;
        bool altitude_valid;
        control_get(&pitch_sp, &roll_sp, &altitude_sp,
                    &altitude_mm, &altitude_valid);

        /* Invert signs to match physical frame orientation of the rig. */
        float pitch = -out.eul.pitch;
        float roll  = -out.eul.roll;

        /* Attitude PID tracks RC-commanded angles (0° = level hover). */
        float pitch_out = pid_update(&pitch_pid, pitch_sp, pitch, dt);
        float roll_out  = pid_update(&roll_pid,  roll_sp,  roll,  dt);

        /* Altitude PID adjusts collective throttle to hold commanded height.
         * Only active once LiDAR has produced a valid measurement. */
        int base = 50;
        float collective = (float)base;
        if (altitude_valid) {
            collective += pid_update(&alt_pid, altitude_sp,
                                     (float)altitude_mm, dt);
        } else {
            pid_reset(&alt_pid);
        }

        int front_left  = (int)collective + (int)pitch_out + (int)roll_out;
        int front_right = (int)collective - (int)pitch_out + (int)roll_out;
        int back_left   = (int)collective + (int)pitch_out - (int)roll_out;
        int back_right  = (int)collective - (int)pitch_out - (int)roll_out;

        if (front_left  < 0) front_left  = 0; else if (front_left  > 100) front_left  = 100;
        if (front_right < 0) front_right = 0; else if (front_right > 100) front_right = 100;
        if (back_left   < 0) back_left   = 0; else if (back_left   > 100) back_left   = 100;
        if (back_right  < 0) back_right  = 0; else if (back_right  > 100) back_right  = 100;

        set_led_intensity(TOP_LEFT,     front_left);
        set_led_intensity(TOP_RIGHT,    front_right);
        set_led_intensity(BOTTOM_LEFT,  back_left);
        set_led_intensity(BOTTOM_RIGHT, back_right);

        /* Throttled telemetry (~2 Hz), integer fixed-point so no %f printk. */
        if (++print_counter >= 10) {
            print_counter = 0;
            char ps, rs, ys, spp, spr;
            int pw, pf, rw, rf, yw, yf, ppw, ppf, prw, prf;
            fx2(pitch,       &ps,  &pw,  &pf);
            fx2(roll,        &rs,  &rw,  &rf);
            fx2(out.eul.yaw, &ys,  &yw,  &yf);
            fx2(pitch_sp,    &spp, &ppw, &ppf);
            fx2(roll_sp,     &spr, &prw, &prf);
            printk("[%s] P:%c%d.%02d R:%c%d.%02d Y:%c%d.%02d | sp P:%c%d.%02d R:%c%d.%02d | alt:%umm sp:%d%s | rx:%u | mix FL:%d FR:%d BL:%d BR:%d\n",
                   now_str(),
                   ps, pw, pf,  rs, rw, rf,  ys, yw, yf,
                   spp, ppw, ppf,  spr, prw, prf,
                   altitude_mm, (int)altitude_sp,
                   altitude_valid ? "" : "(off)",
                   uart_rx_count,
                   front_left, front_right, back_left, back_right);
        }

        k_msleep(50);  /* 20 Hz control loop */
    }
}

static void lidar_consumer(void *arg1, void *arg2, void *arg3) {
    struct lidar_data out; 
    printk("LiDAR consumer thread started\n");
    
    while (1) {
        int rc = k_msgq_get(&lidar_msgq, &out, K_FOREVER);
        if (rc == 0) {
            /* Publish altitude to the control loop (engages altitude hold). */
            control_set_altitude(out.distance_mm);
            /* Proximity warning LED. */
            if(out.distance_mm < 300) {
                gpio_pin_set_dt(&led0, 1);
            } else {
                gpio_pin_set_dt(&led0, 0);
            }
        } else {
            printk("Failed to get LiDAR data: %d\n", rc);
        }
        k_msleep(100);
    }
}

static void uart_consumer(void *arg1,  void *arg2, void *arg3) {
    struct uart_msg out;
    printk("User Control thread started\n");
    while(1)
    {
        if(k_msgq_get(&uart_rx_msgq, &out, K_FOREVER) == 0)
        {
            char c = out.data[0];
            uart_rx_count++;
                switch (c) {
                case 'L':
                    control_adjust_roll(-CONTROL_ATTITUDE_STEP_DEG);
                    printk("UART: LEFT  (roll setpoint down)\n");
                    break;
                case 'R':
                    control_adjust_roll(+CONTROL_ATTITUDE_STEP_DEG);
                    printk("UART: RIGHT (roll setpoint up)\n");
                    break;
                case 'U':
                    control_adjust_pitch(+CONTROL_ATTITUDE_STEP_DEG);
                    printk("UART: UP    (pitch setpoint up)\n");
                    break;
                case 'D':
                    control_adjust_pitch(-CONTROL_ATTITUDE_STEP_DEG);
                    printk("UART: DOWN  (pitch setpoint down)\n");
                    break;
                case 'A':
                    control_adjust_altitude(+CONTROL_ALTITUDE_STEP_MM);
                    printk("UART: ASCEND  (altitude setpoint +%dmm)\n",
                           (int)CONTROL_ALTITUDE_STEP_MM);
                    break;
                case 'X':
                    control_adjust_altitude(-CONTROL_ALTITUDE_STEP_MM);
                    printk("UART: DESCEND (altitude setpoint -%dmm)\n",
                           (int)CONTROL_ALTITUDE_STEP_MM);
                    break;
                default:
                    printk("UART: UNKNOWN '%c'\n", c);
            }
        }
    }
}

/* Probe every 7-bit address on a bus and print which ones ACK. Lets you tell a
 * miswired-but-alive sensor (shows up at its address) from a dead/unwired one
 * (nothing responds). */
static void i2c_scan(const struct device *bus, const char *label)
{
    if (!device_is_ready(bus)) {
        printk("i2c_scan: %s not ready\n", label);
        return;
    }
    printk("Scanning %s...\n", label);
    int found = 0;
    for (uint8_t addr = 0x08; addr <= 0x77; addr++) {
        uint8_t dummy;
        if (i2c_read(bus, &dummy, 1, addr) == 0) {
            printk("  ACK at 0x%02x\n", addr);
            found++;
        }
    }
    printk("  %s: %d device(s) responded\n", label, found);
}

int main(void)
{
    // printk("Checking I2C devices...\n");

    const struct device *i2c0_dev = DEVICE_DT_GET(DT_NODELABEL(i2c0));
    const struct device *i2c1_dev = DEVICE_DT_GET(DT_NODELABEL(i2c1));
    int rc = gpio_pin_configure_dt(&led0, GPIO_OUTPUT_ACTIVE); 
    if(rc != 0) {
        printk("Failed to configure LED GPIO: %d\n", rc);
        return -1;
    }
    // Check if GPIO is ready
    if (!device_is_ready(led0.port)) {
        printk("LED GPIO device is not ready\n");
        return -1;
    }
    printk("setting to 0\n"); 
    gpio_pin_set_dt(&led0, 0); 
    
    printk("I2C0 ready: %s\n", device_is_ready(i2c0_dev) ? "YES" : "NO");
    printk("I2C1 ready: %s\n", device_is_ready(i2c1_dev) ? "YES" : "NO");

    /* Diagnostic: list every device that ACKs on each bus. BNO055 should
     * appear at 0x28 (or 0x29 if ADR is high); LiDAR at 0x62. */
    i2c_scan(i2c0_dev, "i2c0 (BNO055 @0x28)");
    i2c_scan(i2c1_dev, "i2c1 (LiDAR @0x62)");
    
    printk("Starting LiDAR-Lite V4 and BNO test app...\n");

    //Initialize BNO
    const struct device *const bno = DEVICE_DT_GET(DT_NODELABEL(bno));
    rc = bno_init(bno);
    if (rc != 0) {
        /* Non-fatal: keep booting so LiDAR/UART/LEDs still run and print.
         * Attitude control is simply disabled until the IMU comes up. */
        printk("bno_init failed: %d — continuing without IMU\n", rc);
    }

    // // Initialize LiDAR
    const struct device *const lidar_device = DEVICE_DT_GET(DT_NODELABEL(lidar));
    rc = lidar_init(lidar_device); 
    if(rc != 0) { 
        printk("lidar_init failed: %d\n", rc);
        // return rc;
    }

    //Initialize UART
    const struct device *const uart_control = DEVICE_DT_GET(DT_NODELABEL(uart1));
    int uc = uart_init(uart_control);
    if(uc != 0){
        printk("uart_init failed: %d\n",uc);
    }
    initialize_leds(); 
    printk("Leds initialized\n");

    control_init();
    printk("Control setpoints initialized\n");



    // Start state machine thread
    k_thread_create(&state_machine_thread_data, state_machine_stack, STACK_SIZE,
                    state_machine_thread, NULL, NULL, NULL,
                    COMMS_PRIORITY, 0, K_NO_WAIT);

    // Start BNO consumer thread (the one that prints data)
    k_thread_create(&bno_consumer_thread_data, 
                    bno_consumer_stack, 
                    BNO_STACK_SIZE, 
                    bno_consumer, 
                    NULL, NULL, NULL, 
                    BNO_PRIORITY, 0, K_NO_WAIT);
     // Start LiDAR consumer thread (the one that prints data)
    k_thread_create(&lidar_consumer_thread_data, 
                    lidar_consumer_stack, 
                    LIDAR_STACK_SIZE, 
                    lidar_consumer, 
                    NULL, NULL, NULL, 
                    LIDAR_PRIORITY, 0, K_NO_WAIT);

    // Start UART consumer thread (the one that prints data)
    k_thread_create(&uart_consumer_thread_data, 
                    uart_consumer_stack, 
                    UART_CONSUMER_STACK_SIZE, 
                    uart_consumer, 
                    NULL, NULL, NULL,
                    UART_CONSUMER_PRIORITY, 0, K_NO_WAIT);

    printk("All threads started\n");

    // Start LiDAR timer after everything is initialized
    k_timer_start(&lidar_timer, K_MSEC(30), K_MSEC(30));
    
    printk("LiDAR timer started, monitoring data...\n");
    // while(1) { 
    //     for(int i = 0; i < 100; i++) { 
    //         set_led_intensity(TOP_LEFT, i); 
    //         // set_led_intensity(TOP_RIGHT, i); 
    //         set_led_intensity(BOTTOM_LEFT, i); 
    //         // set_led_intensity(BOTTOM_RIGHT, i); 
    //         k_msleep(10);
    //     }
    //     k_msleep(1000); 
    //     for(int i = 100; i>=0; i--) { 
    //         set_led_intensity(TOP_LEFT, i); 
    //         // set_led_intensity(TOP_RIGHT, i); 
    //         set_led_intensity(BOTTOM_LEFT, i); 
    //         // set_led_intensity(BOTTOM_RIGHT, i); 
    //         k_msleep(10);
    //     }
    // }
    return 0;
}
