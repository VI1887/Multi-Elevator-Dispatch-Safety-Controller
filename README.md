# Multi-Elevator Dispatch & Safety Controller

A console C program that simulates a bank of elevator cars deciding, for a
batch of target-floor requests, which car should serve each one safely and
efficiently.

The only file this program reads is `data/config.txt`, and it holds fixed
setup limits only:

```
numCars=2
numFloors=11
maxTargets=10
```

## The program runs, in this order:

1. **Each elevator's current floor.**
2. **Each elevator's sensor readings** (fire alarm, emergency stop, load
   percent, door obstructed, manual hold) - fed straight into
   `safety.c`'s `decide_car_safety_state()`, which decides that car's
   state. A car that comes back `OUT_OF_SERVICE` is excluded from dispatch
   for the rest of that run.
3. **How many target-floor requests there are**, then for each one: the
   floor, and the direction the waiting passenger wants to travel (UP or
   DOWN) - exactly like a real hall-call button.

Only after all of that is collected does the program start deciding
anything. `main.c` never makes a safety, dispatch, or scheduling decision
itself - it only collects input and calls the module responsible.

## How the dispatch decision works

Given a batch of target-floor requests collected up front (nothing has
moved yet), `assign_calls_batch()` in `dispatch.c` processes them in the
order you entered them and scores every in-service car for each one:

- A car **already committed** to the same direction as this call scores
  best (just its distance) - this is what groups multiple same-direction
  calls onto one elevator.
- A car **not yet committed to any direction** scores next best.
- A car committed to the **opposite** direction is a last resort.

Whichever car scores lowest gets the call, and becomes committed to that
call's direction for the rest of the batch.

**Worked example** (2 cars, both starting at floor 0, 11 floors):
floor 4 DOWN, floor 6 DOWN, floor 1 UP →

```
Target 1 (floor 4, DOWN): assigned to Elevator 0.
Target 2 (floor 6, DOWN): assigned to Elevator 0.
Target 3 (floor 1, UP):   assigned to Elevator 1.
```

Once every call is assigned, `sort_destinations_for_sweep()` in
`scheduler.c` orders each car's queued stops into one coherent sweep:

- A **DOWN**-committed car visits its stops **highest floor first**, then
  sweeps downward (so Elevator 0 above visits floor 6, then floor 4 -
  exactly how a real elevator picks up everyone heading down on its way
  past them, instead of darting back and forth).
- An **UP**-committed car visits its stops **lowest floor first**.

`main.c` then steps through each car's sorted stop list, printing and
logging every stop, and tallies floor-traffic counts along the way.

## Project layout

```
include/      *.h headers for every module
src/          *.c implementations, including main.c
data/         config.txt (the only input file); elevator_log.txt and
              summary_report.txt are written by a run, not read
tests/        test_core.cpp - Google Test unit tests for the core
              decision functions
docs/         Phase 1/2 project documentation
Makefile
```

## Building and running

```
make                       # builds elevator_sim.exe
make run                   # builds (if needed) and runs it
./elevator_sim.exe         # or run it directly
make test                  # builds and runs the Google Test suite (needs libgtest-dev)
make clean
```

Menu once it's running:
```
1. Run simulation
2. View last report
3. Exit
```

## Unit tests

`tests/test_core.cpp` is a Google Test suite (18 tests) covering normal /
invalid / edge cases for:

- `decide_car_safety_state` - priority order, NULL handling, state recovery
- `add_destination` - normal add, out-of-range floor, full queue, NULL car
- `get_next_stop` - same-direction pick, reversal, empty queue, NULL car
- `sort_destinations_for_sweep` - DOWN/UP ordering, single-stop no-op
- `assign_calls_batch` - the worked example from above, and "no car available"


## Judgment calls worth knowing about

- There's no dedicated `OVERLOAD` state in the `CarState` enum, so an
  overloaded car (load >= 100%) is put into `DOOR_OPEN` - doors stay open
  and the car won't move until the load is corrected, matching a real
  installation's overload buzzer-and-hold behaviour.
- Because every call is known before any car moves, dispatch is a batch
  decision rather than a live, one-call-at-a-time decision. This is
  realistic for "collective control" systems that gather calls and decide
  routes rather than greedily picking the nearest car for each call in
  isolation.
- The highest/lowest-first sweep ordering assumes a car's queued calls are
  all reachable in one monotonic sweep from wherever it currently sits.
  For the kind of scenario this project is meant to demonstrate (a car
  serving a cluster of same-direction calls), that holds; it is a
  deliberate simplification rather than a full real-time motion model.
