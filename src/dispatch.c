#include <stdlib.h>
#include "dispatch.h"
#include "scheduler.h"

static int batch_score(const ElevatorCar *car, int callFloor, Direction callDirection) {
    const int IDLE_PENALTY = 1000;
    const int WRONG_DIR_PENALTY = 100000;

    int distance = abs(car->currentFloor - callFloor);

    if (car->direction == NONE) {
        return distance + IDLE_PENALTY;
    }
    if (car->direction == callDirection) {
        return distance;
    }
    return distance + WRONG_DIR_PENALTY;
}

int assign_calls_batch(ElevatorCar cars[], int numCars, CallRequest calls[],
                        int numCalls) {
    if (cars == NULL || calls == NULL) {
        return numCalls; /* nothing could be assigned */
    }

    int unassigned = 0;

    for (int c = 0; c < numCalls; c++) {
        int bestId = -1;
        int bestScore = -1;

        for (int i = 0; i < numCars; i++) {
            if (!cars[i].inService || cars[i].state == OUT_OF_SERVICE) {
                continue;
            }
            int score = batch_score(&cars[i], calls[c].callFloor, calls[c].direction);
            if (bestId == -1 || score < bestScore) {
                bestId = i;
                bestScore = score;
            }
        }

        if (bestId == -1) {
            calls[c].assignedCarId = -1;
            calls[c].distanceAtAssignment = -1;
            unassigned++;
            continue;
        }

        calls[c].assignedCarId = bestId;
        calls[c].distanceAtAssignment = abs(cars[bestId].currentFloor - calls[c].callFloor);


        cars[bestId].direction = calls[c].direction;
        cars[bestId].state = (calls[c].direction == UP) ? MOVING_UP : MOVING_DOWN;
        add_destination(&cars[bestId], calls[c].callFloor);
    }

    return unassigned;
}