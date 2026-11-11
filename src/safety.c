#include "safety.h"

int decide_car_safety_state(bool emergency_stop, bool fire_alarm,
                             bool door_obstructed, int load_percent,
                             bool manual_hold, CarState current_state,
                             CarState *out_state) {
    if (out_state == NULL) {
        return -1;
    }

    /* Clamp defensively - callers are expected to clamp too, but a safety
     * function should never trust its inputs blindly. */
    if (load_percent < 0) {
        load_percent = 0;
    }
    if (load_percent > 100) {
        load_percent = 100;
    }

    bool overload = (load_percent >= 100);

    if (fire_alarm) {
        *out_state = OUT_OF_SERVICE;
    } else if (emergency_stop) {
        *out_state = OUT_OF_SERVICE;
    } else if (overload) {
        *out_state = DOOR_OPEN;
    } else if (door_obstructed) {
        *out_state = DOOR_OPEN;
    } else if (manual_hold) {
        *out_state = DOOR_OPEN;
    } else {
        /* No safety condition is active. If the car had been held at
         * DOOR_OPEN or OUT_OF_SERVICE purely because of a flag that has now
         * cleared, it returns to normal service (IDLE). Any other state
         * (MOVING_UP / MOVING_DOWN / IDLE) belongs to the scheduler and is
         * left untouched. */
        if (current_state == DOOR_OPEN || current_state == OUT_OF_SERVICE) {
            *out_state = IDLE;
        } else {
            *out_state = current_state;
        }
    }

    return 0;
}
