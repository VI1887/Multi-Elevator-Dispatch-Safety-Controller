# Phase 2: Detailed Design

## Project Title
Multi-Elevator Dispatch & Safety Controller

## Student Information
Vaibhav Inamdar, Roll 612509059, Instrumentation & Control, Batch 1.

## 1. Overview

This document works out the details behind the Phase 1 proposal: the actual data structures, the function signatures for each module, and the logic each function follows. The program itself is a console simulation. Apart from one small configuration file, every input - each car's current floor, each car's sensor readings, and every target-floor request - is typed in directly by the person running the program. The modules don't know or care where the numbers came from; they just take values and decide things.

## 2. File Layout

```
include/   header files - one per module
src/       source files, including main.c
data/      config.txt (the only file the program reads); elevator_log.txt
           and summary_report.txt are written by a run, not read
tests/     test_core.cpp - Google Test unit tests
docs/      this document and the Phase 1 proposal
Makefile
```

## 3. Data Structures

All of these live in `common.h`, since every module needs them.

### 3.1 Enumerations

```c
typedef enum { UP, DOWN, NONE } Direction;

typedef enum {
    IDLE,
    MOVING_UP,
    MOVING_DOWN,
    DOOR_OPEN,
    OUT_OF_SERVICE
} CarState;

```

`Direction` is used two ways in this project: as the direction a waiting passenger wants to travel (part of a `CallRequest`), and as the direction a car is currently committed to serving (part of `ElevatorCar`). `NONE` means "not committed to anything yet."

There's no separate `OVERLOAD` state. An overloaded car is put into `DOOR_OPEN` instead, since in both cases the doors stay open and the car doesn't move until the condition clears - the same as a real installation's overload buzzer-and-hold behaviour.

### 3.2 `ElevatorCar`

```c
typedef struct {
    int carId;
    int currentFloor;
    Direction direction;
    CarState state;
    int destinationFloors[MAX_STOPS];
    int numDestinations;
    int loadPercent;
    bool emergencyStop;
    bool fireAlarm;
    bool doorObstructed;
    bool manualHold;
    bool inService;
} ElevatorCar;
```

`direction` here is overloaded on purpose: once a car has been assigned at least one request, this field holds the direction it's now committed to serving, and stays that way until the sweep finishes. `inService` turns false once a car's safety state comes back `OUT_OF_SERVICE`, so it gets skipped by dispatch for the rest of that run.

### 3.3 `CallRequest`

```c
typedef struct {
    int callFloor;
    Direction direction;
    int assignedCarId;
    int distanceAtAssignment;
} CallRequest;
```

This is one target-floor request - the floor and the direction the waiting passenger wants to go, exactly like pressing a real hall-call button. `assignedCarId` and `distanceAtAssignment` start at -1 and get filled in once the dispatch module has made its decision.

### 3.4 Constants

```c
#define MAX_CARS         8
#define MAX_FLOORS       50
#define MAX_STOPS        20
#define MAX_CALLS        20
#define MAX_LINE_LENGTH  200
```

These cap the size of every array in the project, so nothing needs dynamic memory.

## 4. Module-by-Module Design

### 4.1 `input` - `load_config()`

```c
int load_config(const char *filename, int *out_numCars, int *out_numFloors, int *out_maxTargets);
```

Reads `data/config.txt`, which holds three lines: `numCars`, `numFloors`, `maxTargets`. Blank lines and lines starting with `#` are skipped. Returns 0 on success, and a negative code if a pointer was null, the file couldn't be opened, or the file's contents didn't make sense (missing field or invalid value). Values are clamped to the project's constants if the file asks for more than the arrays can hold.

This is the only file-reading function in the whole project. Nothing else touches the filesystem for input.

### 4.2 `safety` - `decide_car_safety_state()`

```c
int decide_car_safety_state(bool emergency_stop, bool fire_alarm,
                             bool door_obstructed, int load_percent,
                             bool manual_hold, CarState current_state,
                             CarState *out_state);
```

Takes a car's five sensor flags plus its current state, and decides what its state should be. The priority order, highest first:

1. Fire alarm → `OUT_OF_SERVICE`
2. Emergency stop → `OUT_OF_SERVICE`
3. Overload (load percent at or above 100) → `DOOR_OPEN`
4. Door obstructed → `DOOR_OPEN`
5. Manual hold → `DOOR_OPEN`
6. None of the above → if the car was being held at `DOOR_OPEN` or `OUT_OF_SERVICE` purely because of a flag that's now cleared, it goes back to `IDLE`; otherwise its state is left as it was.

`load_percent` is clamped to the 0-100 range inside the function itself, so it never trusts the caller to have already done that. Returns -1 only if `out_state` is a null pointer.

### 4.3 `scheduler` - stop queue and visiting order

```c
int add_destination(ElevatorCar *car, int floor);
int get_next_stop(ElevatorCar *car, int *out_nextFloor);
void sort_destinations_for_sweep(ElevatorCar *car);
```

**`add_destination`** pushes a floor onto a car's queue, rejecting a null car, a full queue, or an out-of-range floor. It silently ignores a floor that's already queued rather than adding a duplicate.

**`get_next_stop`** picks the nearest queued stop that's still ahead of the car in its current direction; if nothing qualifies, it reverses and picks the nearest stop overall. This was written for a live, step-by-step simulation where the car's position updates between calls.

**`sort_destinations_for_sweep`** is what actually makes sense of a car's whole queue once every request for a run has already been assigned (nothing has moved yet at that point). It orders the queue so a `DOWN`-committed car visits its stops highest floor first, and an `UP`-committed car visits its stops lowest floor first. That's what makes a car serving floor 4 and floor 6 (both wanting to go down) visit floor 6 first and then floor 4 on the way back down, instead of bouncing between them. It does nothing if there are fewer than two stops queued, or the car isn't committed to a direction yet.

### 4.4 `dispatch` - deciding which car answers which request

```c
int assign_calls_batch(ElevatorCar cars[], int numCars, CallRequest calls[], int numCalls);
```

**`assign_calls_batch`** is the function `main.c` actually calls. It processes the batch of requests in the order they were entered. For each one, it scores every in-service car:

- A car already committed to the *same* direction as this request scores just its distance - the best possible fit, since the car can simply add this stop to its sweep.
- A car not yet committed to any direction scores its distance plus a fixed penalty - still usable, just not as good a fit as a car already heading that way.
- A car committed to the *opposite* direction scores a much larger penalty - a last resort, only picked if nothing better is available.

Whichever car scores lowest gets the request. That car's `direction` and `state` are updated to reflect the commitment, and the floor is pushed onto its queue with `add_destination`. This is what naturally groups same-direction requests onto one car while keeping a different direction's requests on another - the dispatch decision doesn't need to know in advance which requests belong together; the scoring takes care of it one request at a time.


### 4.5 `report` - logging and the summary

```c
int log_event(FILE *logFile, const char *description, int carId, int floor);
void generate_summary(FILE *out, const CallRequest calls[], int numCalls,
                       const int floorVisits[], int numFloors,
                       const int overrideCounts[5]);
```

**`log_event`** writes one line per event to `data/elevator_log.txt` as the run happens.

**`generate_summary`** prints, for every target-floor request, which car answered it and how far that car had to travel to reach it (or that no car was available); then how many times each floor was visited; then how many times each safety condition was triggered across all cars. It writes to whatever `FILE*` it's given, so the same function is used both for the on-screen summary and for saving `data/summary_report.txt`.

### 4.6 `main` - collecting input and running the modules in order

`main.c` does not decide anything itself. Its job, in order:

1. Load the configuration.
2. Ask for each car's current floor.
3. Ask for each car's sensor readings, and immediately call `decide_car_safety_state` for that car before moving to the next one.
4. Ask how many target-floor requests there are, then ask for each one's floor and direction.
5. Call `assign_calls_batch` once, with the whole batch of requests.
6. For every in-service car with stops queued, call `sort_destinations_for_sweep`, then repeatedly call `get_next_stop` until the queue is empty, printing and logging each stop along the way.
7. Call `generate_summary`, once for the screen and once for the saved report file.

The menu loop around all of this (`run simulation` / `view last report` / `exit`) lives in `main()` itself.

## 5. Input Validation

Every value typed in - a floor number, a 0/1 sensor answer, a direction choice, a menu choice - goes through the same small helper in `main.c` that keeps re-prompting until the number given is both a valid integer and inside the expected range, rather than trusting the first thing that comes back from `scanf`. Load percent is accepted up to 150 and then clamped to 100, since a sensor reporting "150% full" should still be treated as simply overloaded rather than rejected outright.

## 6. Error Handling

Every function that can fail returns a status code rather than crashing or printing an error itself - 0 for success, and a specific negative number for each kind of failure (null pointer, out-of-range value, nothing available, and so on). `main.c` is the only place that turns those codes into something the person running the program actually sees.

## 7. Test Plan

`tests/test_core.cpp` uses Google Test and covers, with a normal case, an invalid case, and at least one edge case for each:

- `decide_car_safety_state` - normal (no flags), priority order between fire alarm and overload, recovery back to `IDLE`, and a null output pointer.
- `add_destination` - normal add, an out-of-range floor, a full queue, and a null car.
- `get_next_stop` - nearest stop in the current direction, reversing when nothing is left ahead, an empty queue, and a null car.
- `sort_destinations_for_sweep` - `DOWN` sorts highest-first, `UP` sorts lowest-first, and a single stop is left alone.
- `assign_calls_batch` - the grouping behaviour described in section 4.4 (two same-direction requests landing on one car, a different-direction request landing on the other), and the case where no car is available at all.

18 test cases in total, across 5 test suites.

## 8. Known Limitations

- The sweep ordering in `sort_destinations_for_sweep` assumes a car's queued stops can all be reached in one straight pass from wherever it currently is. That holds for the scenarios this project is meant to demonstrate, but it's a deliberate simplification rather than a full real-time motion model.
