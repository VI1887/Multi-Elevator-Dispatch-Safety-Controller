#include <stdlib.h>
#include "scheduler.h"

int add_destination(ElevatorCar *car, int floor) {
    if (car == NULL) {
        return -1;
    }
    if (car->numDestinations >= MAX_STOPS) {
        return -2;
    }
    if (floor < 0 || floor >= MAX_FLOORS) {
        return -3;
    }

    /* avoid queuing the same floor twice */
    for (int i = 0; i < car->numDestinations; i++) {
        if (car->destinationFloors[i] == floor) {
            return 0;
        }
    }

    car->destinationFloors[car->numDestinations] = floor;
    car->numDestinations++;
    return 0;
}

static void remove_stop_at(ElevatorCar *car, int index) {
    for (int i = index; i < car->numDestinations - 1; i++) {
        car->destinationFloors[i] = car->destinationFloors[i + 1];
    }
    car->numDestinations--;
}

int get_next_stop(ElevatorCar *car, int *out_nextFloor) {
    if (car == NULL || out_nextFloor == NULL) {
        return -2;
    }
    if (car->numDestinations == 0) {
        return -1;
    }

    /* Prefer the nearest stop that is in the car's current direction of
     * travel, so it finishes sweeping that direction before reversing. */
    int bestIndex = -1;
    int bestDistance = -1;

    for (int i = 0; i < car->numDestinations; i++) {
        int stop = car->destinationFloors[i];
        bool sameDirection =
            (car->direction == UP && stop >= car->currentFloor) ||
            (car->direction == DOWN && stop <= car->currentFloor);

        if (car->direction == NONE || sameDirection) {
            int distance = abs(stop - car->currentFloor);
            if (bestIndex == -1 || distance < bestDistance) {
                bestIndex = i;
                bestDistance = distance;
            }
        }
    }

    /* Nothing left in the current direction: reverse and take the nearest
     * stop overall. */
    if (bestIndex == -1) {
        for (int i = 0; i < car->numDestinations; i++) {
            int distance = abs(car->destinationFloors[i] - car->currentFloor);
            if (bestIndex == -1 || distance < bestDistance) {
                bestIndex = i;
                bestDistance = distance;
            }
        }
    }

    *out_nextFloor = car->destinationFloors[bestIndex];
    remove_stop_at(car, bestIndex);
    return 0;
}

void sort_destinations_for_sweep(ElevatorCar *car) {
    if (car == NULL || car->numDestinations < 2 || car->direction == NONE) {
        return;
    }

    /* small fixed-size array (MAX_STOPS) - a simple insertion sort is plenty */
    for (int i = 1; i < car->numDestinations; i++) {
        int key = car->destinationFloors[i];
        int j = i - 1;

        if (car->direction == DOWN) {
            /* descending: highest floor first */
            while (j >= 0 && car->destinationFloors[j] < key) {
                car->destinationFloors[j + 1] = car->destinationFloors[j];
                j--;
            }
        } else { /* UP */
            /* ascending: lowest floor first */
            while (j >= 0 && car->destinationFloors[j] > key) {
                car->destinationFloors[j + 1] = car->destinationFloors[j];
                j--;
            }
        }
        car->destinationFloors[j + 1] = key;
    }
}
