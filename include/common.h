#ifndef COMMON_H
#define COMMON_H

#include <stdbool.h>
#include <stdio.h>

#define MAX_CARS         8
#define MAX_FLOORS       50
#define MAX_STOPS        20
#define MAX_CALLS        20    /* most target-floor requests accepted in one run */
#define MAX_LINE_LENGTH  200

typedef enum { UP, DOWN, NONE } Direction;

typedef enum {
    IDLE,
    MOVING_UP,
    MOVING_DOWN,
    DOOR_OPEN,
    OUT_OF_SERVICE
} CarState;


typedef struct {
    int carId;
    int currentFloor;
    Direction direction;     /* once assigned a call, the direction this car
                               * is now committed to serving (UP or DOWN);
                               * NONE while idle/unassigned */
    CarState state;
    int destinationFloors[MAX_STOPS];
    int numDestinations;
    int loadPercent;
    bool emergencyStop;
    bool fireAlarm;
    bool doorObstructed;
    bool manualHold;
    bool inService;      //false once the car has been marked OUT_OF_SERVICE
} ElevatorCar;

typedef struct {
    int callFloor;
    Direction direction;
    int assignedCarId;
    int distanceAtAssignment;
} CallRequest;

#endif
