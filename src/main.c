#include <stdio.h>
#include <stdlib.h>
#include "common.h"
#include "input.h"
#include "safety.h"
#include "dispatch.h"
#include "scheduler.h"
#include "report.h"

#define CONFIG_FILE  "data/config.txt"
#define LOG_FILE     "data/elevator_log.txt"
#define REPORT_FILE  "data/summary_report.txt"


static int read_int_input(const char *prompt, int minVal, int maxVal) {
    int value;
    while (1) {
        printf("%s (%d-%d): ", prompt, minVal, maxVal);
        if (scanf("%d", &value) == 1 && value >= minVal && value <= maxVal) {
            return value;
        }
        printf("  Invalid input - please enter a whole number between %d and %d.\n",
               minVal, maxVal);
        int c;
        while ((c = getchar()) != '\n' && c != EOF) { /* clear bad input */ }
    }
}

static bool read_bool_input(const char *prompt) {
    return read_int_input(prompt, 0, 1) == 1;
}

static Direction read_direction_input(const char *prompt) {
    printf("%s\n", prompt);
    int choice = read_int_input("  0 = UP, 1 = DOWN", 0, 1);
    return (choice == 0) ? UP : DOWN;
}


static void init_cars(ElevatorCar cars[], int numCars) {
    for (int i = 0; i < numCars; i++) {
        cars[i].carId = i;
        cars[i].currentFloor = 0;
        cars[i].direction = NONE;
        cars[i].state = IDLE;
        cars[i].numDestinations = 0;
        cars[i].loadPercent = 0;
        cars[i].emergencyStop = false;
        cars[i].fireAlarm = false;
        cars[i].doorObstructed = false;
        cars[i].manualHold = false;
        cars[i].inService = true;
    }
}

static const char *state_name(CarState state) {
    switch (state) {
        case IDLE:            return "IDLE";
        case MOVING_UP:       return "MOVING_UP";
        case MOVING_DOWN:     return "MOVING_DOWN";
        case DOOR_OPEN:       return "DOOR_OPEN";
        case OUT_OF_SERVICE:  return "OUT_OF_SERVICE";
        default:              return "UNKNOWN";
    }
}


static void collect_current_floors(ElevatorCar cars[], int numCars, int numFloors) {
    printf("\n--- Current position of each elevator ---\n");
    for (int i = 0; i < numCars; i++) {
        char prompt[64];
        snprintf(prompt, sizeof(prompt), "Elevator %d - current floor", i);
        cars[i].currentFloor = read_int_input(prompt, 0, numFloors - 1);
    }
}

static void collect_sensor_readings(ElevatorCar cars[], int numCars, int overrideCounts[5]) {
    printf("\n--- Sensor readings for each elevator ---\n");
    for (int i = 0; i < numCars; i++) {
        ElevatorCar *car = &cars[i];
        printf("\nElevator %d (at floor %d):\n", car->carId, car->currentFloor);

        car->fireAlarm      = read_bool_input("  Fire alarm active?");
        car->emergencyStop  = read_bool_input("  Emergency stop pressed?");
        car->loadPercent    = read_int_input("  Current load percent", 0, 150);
        if (car->loadPercent > 100) car->loadPercent = 100; /* clamp, FR-11 */
        car->doorObstructed = read_bool_input("  Door obstructed?");
        car->manualHold      = read_bool_input("  Manual hold engaged?");

        if (car->fireAlarm)          overrideCounts[OV_FIRE_ALARM]++;
        if (car->emergencyStop)      overrideCounts[OV_EMERGENCY_STOP]++;
        if (car->loadPercent >= 100) overrideCounts[OV_OVERLOAD]++;
        if (car->doorObstructed)     overrideCounts[OV_DOOR_OBSTRUCTED]++;
        if (car->manualHold)         overrideCounts[OV_MANUAL_HOLD]++;

        CarState newState;
        decide_car_safety_state(car->emergencyStop, car->fireAlarm, car->doorObstructed,
                                 car->loadPercent, car->manualHold, car->state, &newState);
        car->state = newState;

        printf("  -> Elevator %d safety state: %s\n", car->carId, state_name(car->state));

        if (car->state == OUT_OF_SERVICE) {
            car->inService = false;
            printf("  -> Elevator %d will NOT be used for dispatch this run.\n", car->carId);
        }
    }
}

static int collect_target_calls(CallRequest calls[], int maxTargets, int numFloors) {
    printf("\n--- Target floor requests ---\n");
    int numCalls = read_int_input("How many target floors are there", 1, maxTargets);

    for (int i = 0; i < numCalls; i++) {
        printf("\nTarget %d:\n", i + 1);
        char floorPrompt[32];
        snprintf(floorPrompt, sizeof(floorPrompt), "  Floor");
        calls[i].callFloor = read_int_input(floorPrompt, 0, numFloors - 1);
        calls[i].direction = read_direction_input("  Direction requested:");
        calls[i].assignedCarId = -1;
        calls[i].distanceAtAssignment = -1;
    }
    return numCalls;
}


static void print_dispatch_results(const CallRequest calls[], int numCalls) {
    printf("\n--- Dispatch decisions ---\n");
    for (int i = 0; i < numCalls; i++) {
        const char *dirWord = (calls[i].direction == UP) ? "UP" : "DOWN";
        if (calls[i].assignedCarId == -1) {
            printf("Target %d (floor %d, %s): no elevator available.\n",
                   i + 1, calls[i].callFloor, dirWord);
        } else {
            printf("Target %d (floor %d, %s): assigned to Elevator %d.\n",
                   i + 1, calls[i].callFloor, dirWord, calls[i].assignedCarId);
        }
    }
}


static void run_car_sweep(ElevatorCar *car, int floorVisits[], FILE *logFile) {
    if (car->numDestinations == 0) {
        return;
    }

    sort_destinations_for_sweep(car);

    printf("\nElevator %d sweep (starting at floor %d):\n", car->carId, car->currentFloor);

    int nextFloor;
    while (get_next_stop(car, &nextFloor) == 0) {
        car->direction = (nextFloor > car->currentFloor) ? UP :
                          (nextFloor < car->currentFloor) ? DOWN : car->direction;
        car->currentFloor = nextFloor;
        floorVisits[nextFloor]++;

        printf("  -> stops at floor %d\n", nextFloor);
        if (logFile) {
            log_event(logFile, "elevator stopped at floor", car->carId, nextFloor);
        }
    }

    car->state = IDLE;
}


static void run_once(int numCars, int numFloors, int maxTargets) {
    ElevatorCar cars[MAX_CARS];
    init_cars(cars, numCars);

    int overrideCounts[5] = {0};
    int floorVisits[MAX_FLOORS] = {0};
    CallRequest calls[MAX_CALLS];

    collect_current_floors(cars, numCars, numFloors);
    collect_sensor_readings(cars, numCars, overrideCounts);
    int numCalls = collect_target_calls(calls, maxTargets, numFloors);

    FILE *logFile = fopen(LOG_FILE, "w");
    if (logFile == NULL) {
        printf("\nWarning: could not open %s for writing - continuing without a saved log.\n",
               LOG_FILE);
    }

    int unassigned = assign_calls_batch(cars, numCars, calls, numCalls);
    print_dispatch_results(calls, numCalls);
    if (unassigned > 0 && logFile) {
        log_event(logFile, "one or more target-floor requests could not be assigned", -1, -1);
    }

    for (int i = 0; i < numCars; i++) {
        if (cars[i].inService) {
            run_car_sweep(&cars[i], floorVisits, logFile);
        }
    }

    printf("\n");
    generate_summary(stdout, calls, numCalls, floorVisits, numFloors, overrideCounts);

    FILE *reportFp = fopen(REPORT_FILE, "w");
    if (reportFp != NULL) {
        generate_summary(reportFp, calls, numCalls, floorVisits, numFloors, overrideCounts);
        fclose(reportFp);
        printf("\nRun finished. Report saved to %s\n", REPORT_FILE);
    } else {
        printf("\nRun finished, but the report file could not be saved.\n");
    }

    if (logFile) fclose(logFile);
}

static void view_last_report(void) {
    FILE *fp = fopen(REPORT_FILE, "r");
    if (fp == NULL) {
        printf("No report has been generated yet. Run the simulation first.\n");
        return;
    }
    char line[MAX_LINE_LENGTH];
    while (fgets(line, sizeof(line), fp) != NULL) {
        fputs(line, stdout);
    }
    fclose(fp);
}


int main(void) {
    int numCars, numFloors, maxTargets;

    int cfgRc = load_config(CONFIG_FILE, &numCars, &numFloors, &maxTargets);
    if (cfgRc != 0) {
        printf("Error: could not load %s (code %d). Nothing to run - exiting.\n",CONFIG_FILE, cfgRc);
        return 1;
    }

    printf("Loaded configuration: %d car(s), %d floor(s), up to %d target ""floor(s) per run.\n",
           numCars, numFloors, maxTargets);
    int choice;
    do
    {
        printf("\nMulti-Elevator Dispatch & Safety Controller\n");
        printf("1. Run simulation\n");
        printf("2. View last report\n");
        printf("3. Exit\n");
        choice = read_int_input("Enter choice", 1, 3);

        switch (choice)
        {
            case 1:
                run_once(numCars, numFloors, maxTargets);
                break;
            case 2:
                view_last_report();
                break;
            case 3:
                break;
        }
    } while (choice != 3);

    return 0;
}
