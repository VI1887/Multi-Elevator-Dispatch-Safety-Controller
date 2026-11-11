#ifndef SAFETY_H
#define SAFETY_H

#include "common.h"

int decide_car_safety_state(bool emergency_stop, bool fire_alarm,bool door_obstructed, int load_percent,bool manual_hold, CarState current_state,CarState *out_state);

#endif
