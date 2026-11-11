#ifndef SCHEDULER_H
#define SCHEDULER_H

#include "common.h"

int get_next_stop(ElevatorCar *car, int *out_nextFloor);

int add_destination(ElevatorCar *car, int floor);

void sort_destinations_for_sweep(ElevatorCar *car);

#endif
