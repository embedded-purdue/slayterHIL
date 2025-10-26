#ifndef STATE_MACHINE_H
#define STATE_MACHINE_H

// types
enum system_events_t {
    TURN_ON_MCU,
    START_FLIGHT,
};

// function declarations
void state_machine_handler(system_events_t);
void state_machine_thread(void *, void *, void *);

#endif