#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>

// define thread
#define STACK_SIZE      2048
#define COMMS_PRIORITY  1
K_THREAD_STACK_DEFINE(state_machine_stack, STACK_SIZE);
static struct k_thread state_machine_thread_data;


// define message queue
#define MESSAGES_PER_QUEUE 10

K_MSGQ_DEFINE(event_msgq, sizeof(system_events_t), MESSAGES_PER_QUEUE, alignof(system_events_t));

int main(void)
{

    k_thread_create(&state_machine_thread_data, state_machine_stack, STACK_SIZE,
                    state_machine_thread, NULL, NULL, NULL,
                    COMMS_PRIORITY, 0, K_NO_WAIT);
	return 0;
}
