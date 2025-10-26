#include <zephyr/kernel.h>
#include "state_machine.h"

void init_hw(void) {
    // Initialize hardware components here
}

void hover(void) {
    // Implementation for hover functionality
}

// define queue of system events
void state_machine_handler(system_events_t event) {

    switch(event) {
        case TURN_ON_MCU:
            init_hw();
            break;
        case START_FLIGHT:
            // Handle event 2
            hover();
            break;
        case FLYING:
            // Handle unknown events
            // signal fly

        default:
            break;
    }
}

void state_machine_thread(void *, void *, void *) {
    for(;;) {
        // Thread loop code here
        system_events_t event;
        while (k_msgq_get(&event_msgq, &event, K_FOREVER) == 0) {
            state_machine_handler(event);
        }
    }
}
