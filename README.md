# Multi-Elevator Dispatch & Safety Controller

## 1. Project Title

**Multi-Elevator Dispatch & Safety Controller**

## 2. Engineering Domain

**Transportation / Building Infrastructure Automation**

## 3. Problem Statement

In large commercial, residential, and hospital buildings with a bank of elevators sharing common hall call buttons, two critical engineering questions arise continuously: which elevator car should answer a given floor request, and in what order should each car visit its assigned stops.

A naive controller that simply dispatches whichever car is physically closest—without considering directional commitment or strict safety overrides—leads to:

* **Unsafe operation:** Ambiguous priority when multiple sensor faults (fire alarm, emergency stop, overload, door obstruction, or manual hold) occur simultaneously.
* **Inefficient routing:** Multiple cars moving in the same direction to serve nearby calls, or a single car zig-zagging back and forth instead of completing a single directional sweep.
* **Lack of traceability:** No persistent operational log or summary metrics for facility managers and maintenance technicians to analyze floor traffic and recurring safety faults.

## 4. Proposed Solution

The **Multi-Elevator Dispatch & Safety Controller** is a modular C console application that simulates a bank of elevator cars serving a multi-floor building. Apart from reading fixed setup limits (`numCars`, `numFloors`, `maxTargets`) from `data/config.txt`, the program collects all simulation inputs interactively from the user:

1. **Current Floor Setup:** Queries each car's starting floor.
2. **Safety Priority Evaluation:** Queries each car's five sensor flags (`fireAlarm`, `emergencyStop`, `loadPercent`, `doorObstructed`, `manualHold`) and immediately evaluates its operational state using a strict deterministic priority order before dispatch begins. Any car placed in `OUT_OF_SERVICE` is excluded from dispatch.
3. **Direction-Grouped Batch Dispatch:** Collects a batch of target-floor requests (floor and desired travel direction: `UP` or `DOWN`) and assigns each request to the best available in-service car. Cars already committed to the same travel direction are prioritized over uncommitted or opposite-direction cars, naturally grouping same-direction calls onto one elevator.
4. **Monotonic Sweep Scheduling:** Sorts each car's queued stops so `DOWN`-committed cars visit stops highest-floor-first and `UP`-committed cars visit stops lowest-floor-first, completing a smooth single pass.
5. **Logging & Reporting:** Logs every event to `data/elevator_log.txt` and outputs a comprehensive summary report to both the console and `data/summary_report.txt`.

## 5. Features

* **Configurable System Bounds:** Loads `numCars`, `numFloors`, and `maxTargets` from `data/config.txt`.
* **Deterministic 5-Level Safety Override:** Evaluates Fire Alarm $\rightarrow$ Emergency Stop $\rightarrow$ Overload ($\ge 100\%$) $\rightarrow$ Door Obstruction $\rightarrow$ Manual Hold.
* **Collective Direction-Grouping Dispatch:** Scores cars by directional commitment and distance so same-direction requests are grouped efficiently.
* **Sweep Stop Ordering:** Prevents elevator zig-zagging by ordering stops highest-first (`DOWN`) or lowest-first (`UP`).
* **Persistent Audit Trail:** Generates both a chronological event log (`data/elevator_log.txt`) and an analytical summary report (`data/summary_report.txt`).
* **Defensive Input Validation:** Re-prompts on non-numeric or out-of-range user inputs without crashing or entering an uncontrolled state.

## 6. Functional Requirements Summary

| ID | Requirement Description |
|---|---|
| **FR-01** | The system shall load a configurable number of elevator cars, building floors, and the maximum number of target-floor requests per run from `data/config.txt`. |
| **FR-02** | The system shall ask for and accept each car's current floor directly from the person running the program. |
| **FR-03** | The system shall ask for and accept each car's sensor readings (emergency stop, fire alarm, overload, door obstruction, manual hold) directly from the user. |
| **FR-04** | The system shall decide each car's safety state from its sensor readings, in a fixed priority order, before that car is considered for dispatch. |
| **FR-05** | The system shall accept a configurable number of target-floor requests, each with a floor and a requested travel direction (`UP` or `DOWN`). |
| **FR-06** | The system shall assign each target-floor request to the most suitable in-service car, grouping requests in the same direction onto the same car where appropriate. |
| **FR-07** | The system shall work out the order in which a car visits its assigned stops, so it completes a sweep in one direction before reversing. |
| **FR-08** | The system shall log every processed decision and resulting stop to `data/elevator_log.txt`. |
| **FR-09** | The system shall generate a summary report at the end of a run (`data/summary_report.txt` and console), covering the dispatch outcome of every request, floor visit counts, and safety override counts. |
| **FR-10** | The system shall validate all input (floor numbers in range, valid sensor flags, valid menu choices) and reject invalid input without crashing. |

## 7. Software Architecture

```
+-------------------------------------------------------------------+
|                               USER                                |
|   (Enters car floors, sensor flags, hall calls, and menu choices) |
+---------------------------------+---------------------------------+
                                  |
                                  v
+-------------------------------------------------------------------+
|                   APPLICATION INTERFACE (src/main.c)              |
|     - Menu control loop & validated console input collection      |
|     - High-level workflow orchestration (no business logic)       |
+------+---------------+------------------+------------------+------+
       |               |                  |                  |
       v               v                  v                  v
+------------+  +-------------+   +---------------+  +--------------+
|   input    |  |   safety    |   |   dispatch    |  |    report    |
| (input.c)  |  | (safety.c)  |   | (dispatch.c)  |  |  (report.c)  |
|            |  |             |   |               |  |              |
| Reads      |  | Priority    |   | Batch scoring |  | Event logger |
| config.txt |  | state check |   | & car select  |  | & summary    |
+-----+------+  +-------------+   +-------+-------+  +------+-------+
      |                                   |                 |
      |                                   v                 |
      |                           +---------------+         |
      |                           |   scheduler   |         |
      |                           | (scheduler.c) |         |
      |                           |               |         |
      |                           | Stop queue &  |         |
      |                           | sweep sorting |         |
      |                           +---------------+         |
      v                                                     v
+-------------------------------------------------------------------+
|                        FILE STORAGE (data/)                       |
|  - data/config.txt         (Input: numCars, numFloors, maxTargets)|
|  - data/elevator_log.txt   (Output: Chronological event log)      |
|  - data/summary_report.txt (Output: Run summary & statistics)     |
+-------------------------------------------------------------------+
```

## 8. Module Descriptions

| Module | Files | Responsibility |
|---|---|---|
| **Common Definitions** | `include/common.h` | Defines shared constants (`MAX_CARS`, `MAX_FLOORS`, `MAX_STOPS`, `MAX_CALLS`), enumerations (`Direction`, `CarState`), and structures (`ElevatorCar`, `CallRequest`). |
| **Input Module** | `include/input.h`, `src/input.c` | Implements `load_config()` to parse `numCars`, `numFloors`, and `maxTargets` from `data/config.txt` and clamp values to safe bounds. |
| **Safety Module** | `include/safety.h`, `src/safety.c` | Implements `decide_car_safety_state()` to evaluate fire alarm, emergency stop, overload, door obstruction, and manual hold in strict priority order. |
| **Dispatch Module** | `include/dispatch.h`, `src/dispatch.c` | Implements `assign_calls_batch()` (and internal `batch_score()`) to score cars by directional commitment and distance, assigning each call to the best car. |
| **Scheduler Module** | `include/scheduler.h`, `src/scheduler.c` | Implements `add_destination()`, `sort_destinations_for_sweep()`, and `get_next_stop()` to manage each car's stop queue and visiting order. |
| **Report Module** | `include/report.h`, `src/report.c` | Implements `log_event()` and `generate_summary()` to record simulation activity to `elevator_log.txt` and `summary_report.txt`. |
| **Main Controller** | `src/main.c` | Entry point providing the interactive menu, validated console input helpers, and sequential orchestration of all modules. |

## 9. Folder Structure

```
Multi-Elevator-Dispatch-Safety-Controller/
|-- src/
|   |-- main.c
|   |-- input.c
|   |-- safety.c
|   |-- dispatch.c
|   |-- scheduler.c
|   `-- report.c
|-- include/
|   |-- common.h
|   |-- input.h
|   |-- safety.h
|   |-- dispatch.h
|   |-- scheduler.h
|   `-- report.h
|-- tests/
|   `-- test_core.cpp
|-- data/
|   |-- config.txt
|   |-- elevator_log.txt
|   `-- summary_report.txt
|-- docs/
|   |-- project_proposal.md
|   `-- detailed_design.md
|-- Makefile
|-- README.md
`-- .gitignore
```

## 10. C Concepts Used

* **Structures (`struct`):** `ElevatorCar` and `CallRequest` encapsulate car status, sensor flags, stop queues, and hall call metadata.
* **Enumerations (`enum`):** `Direction` (`UP`, `DOWN`, `NONE`), `CarState` (`IDLE`, `MOVING_UP`, `MOVING_DOWN`, `DOOR_OPEN`, `OUT_OF_SERVICE`), and safety override indices (`OV_FIRE_ALARM` to `OV_MANUAL_HOLD`) improve readability and prevent invalid numeric states.
* **Arrays:** Fixed-capacity arrays store the bank of cars, per-car destination queues (`destinationFloors[MAX_STOPS]`), batch requests, floor traffic counters, and safety override counts.
* **Pointers:** Used for pass-by-reference struct updates (`ElevatorCar *car`), output parameters (`int *out_nextFloor`, `CarState *out_state`), and file streams (`FILE *`).
* **Functions & Modular Programming:** Single-responsibility functions separated across 6 `.c` files and 6 `.h` files.
* **Strings & File Handling:** `fopen()`, `fgets()`, `sscanf()`, `fprintf()`, `fflush()`, and `fclose()` read configuration settings and write logs/reports.
* **Constants (`#define`):** Named constants replace magic numbers across all buffers and array bounds.
* **Status/Error Codes:** Functions return `0` on success and negative integers (`-1`, `-2`, `-3`) on invalid arguments, full queues, or file errors.

## 11. Coding Standards Followed

* **Indentation & Formatting:** Consistent 4-space indentation, one statement per line, and logical blank-line separation across all files.
* **Naming Conventions:** `UPPER_SNAKE_CASE` for constants and enum values, `PascalCase` for `typedef` structs/enums, and descriptive action-oriented names (`decide_car_safety_state`, `assign_calls_batch`, `sort_destinations_for_sweep`) for functions.
* **Header Hygiene:** Every header file uses `#ifndef / #define / #endif` include guards, and `test_core.cpp` wraps C headers in `extern "C"` for C++ Google Test compatibility.
* **Defensive Design:** All public functions check pointer arguments for `NULL` and validate input ranges before mutating state.

## 12. Prerequisites

* **C/C++ Compiler:** `gcc` and `g++` (MinGW-w64 on Windows or GCC on Linux/WSL/macOS).
* **Build Automation:** `mingw32-make` (Windows) or `make` (Linux/WSL/macOS).
* **Unit Testing Framework:** Google Test (`gtest` and `gtest_main`).

## 13. Build Instructions

Clone the repository and compile the application from the project root directory:

```bash
git clone https://github.com/VI1887/Multi-Elevator-Dispatch-Safety-Controller.git
cd Multi-Elevator-Dispatch-Safety-Controller

# On Windows (MinGW):
mingw32-make all

# On Linux / WSL / macOS:
make all
```

## 14. Run Instructions

Run the compiled simulation using `make run` or by executing the binary directly:

```bash
# On Windows (MinGW):
mingw32-make run

# On Linux / WSL / macOS:
make run

# Or run the executable directly:
./elevator_sim.exe
```

## 15. Test Instructions

Build and execute the automated Google Test suite (18 unit tests across 5 test suites):

```bash
# On Windows (MinGW):
mingw32-make test

# On Linux / WSL / macOS:
make test
```

To remove compiled object files and executables:

```bash
mingw32-make clean
```

## 16. Sample Input

With `data/config.txt` configured as:

```text
numCars=2
numFloors=11
maxTargets=10
```

Interactive console inputs for the worked scenario (2 cars starting at floor 0, all sensors normal, and 3 target-floor requests: Floor 4 DOWN, Floor 6 DOWN, Floor 1 UP):

```text
Loaded configuration: 2 car(s), 11 floor(s), up to 10 target floor(s) per run.

Multi-Elevator Dispatch & Safety Controller
1. Run simulation
2. View last report
3. Exit
Enter choice (1-3): 1

--- Current position of each elevator ---
Elevator 0 - current floor (0-10): 0
Elevator 1 - current floor (0-10): 0

--- Sensor readings for each elevator ---

Elevator 0 (at floor 0):
  Fire alarm active? (0-1): 0
  Emergency stop pressed? (0-1): 0
  Current load percent (0-150): 30
  Door obstructed? (0-1): 0
  Manual hold engaged? (0-1): 0
  -> Elevator 0 safety state: IDLE

Elevator 1 (at floor 0):
  Fire alarm active? (0-1): 0
  Emergency stop pressed? (0-1): 0
  Current load percent (0-150): 20
  Door obstructed? (0-1): 0
  Manual hold engaged? (0-1): 0
  -> Elevator 1 safety state: IDLE

--- Target floor requests ---
How many target floors are there (1-10): 3

Target 1:
  Floor (0-10): 4
  Direction requested:
  0 = UP, 1 = DOWN (0-1): 1

Target 2:
  Floor (0-10): 6
  Direction requested:
  0 = UP, 1 = DOWN (0-1): 1

Target 3:
  Floor (0-10): 1
  Direction requested:
  0 = UP, 1 = DOWN (0-1): 0
```

## 17. Sample Output

Exact console output and generated `data/summary_report.txt` for the inputs above:

```text
--- Dispatch decisions ---
Target 1 (floor 4, DOWN): assigned to Elevator 0.
Target 2 (floor 6, DOWN): assigned to Elevator 0.
Target 3 (floor 1, UP): assigned to Elevator 1.

Elevator 0 sweep (starting at floor 0):
  -> stops at floor 6
  -> stops at floor 4

Elevator 1 sweep (starting at floor 0):
  -> stops at floor 1

===== Simulation Summary =====
Target-floor requests and dispatch outcome:
  Floor  4 (DOWN) : Elevator 0 (travelled 4 floor(s) to reach it)
  Floor  6 (DOWN) : Elevator 0 (travelled 6 floor(s) to reach it)
  Floor  1 (UP) : Elevator 1 (travelled 1 floor(s) to reach it)

Floor traffic:
  Floor  1 : 1 visit(s)
  Floor  4 : 1 visit(s)
  Floor  6 : 1 visit(s)

Safety overrides triggered:
  none
===============================

Run finished. Report saved to data/summary_report.txt
```

## 18. Unit-Testing Summary

`tests/test_core.cpp` implements **18 automated unit test cases** across **5 test suites** using Google Test, covering positive, negative, and boundary conditions:

1. **`SafetyState` (`decide_car_safety_state` — 5 tests):**
   * `NormalNoFlags`: Normal operation with no active flags stays `IDLE`.
   * `FireAlarmHasTopPriority`: Fire alarm overrides simultaneous overload or emergency stop (`OUT_OF_SERVICE`).
   * `OverloadOpensDoor`: `load_percent >= 100` sets state to `DOOR_OPEN`.
   * `RecoversToIdleOnceFlagsClear`: Clears `DOOR_OPEN` and `OUT_OF_SERVICE` back to `IDLE` when flags clear.
   * `NullOutStateRejected`: Returns `-1` when `out_state` is `nullptr`.
2. **`AddDestination` (`add_destination` — 4 tests):**
   * `NormalAdd`: Queues valid floor and increments `numDestinations`.
   * `OutOfRangeFloorRejected`: Returns `-3` for floor `>= MAX_FLOORS`.
   * `FullQueueRejected`: Returns `-2` when `numDestinations == MAX_STOPS`.
   * `NullCarRejected`: Returns `-1` when `car` is `nullptr`.
3. **`GetNextStop` (`get_next_stop` — 4 tests):**
   * `NearestInCurrentDirection`: Picks nearest queued stop ahead in the car's current travel direction.
   * `ReversesWhenNothingAhead`: Picks stop behind the car when no stops remain ahead.
   * `EmptyQueueReturnsError`: Returns `-1` on an empty stop queue.
   * `NullCarRejected`: Returns `-2` when `car` is `nullptr`.
4. **`SortForSweep` (`sort_destinations_for_sweep` — 3 tests):**
   * `DownDirectionHighestFirst`: Sorts a `DOWN`-committed car's stops descending (e.g., `6` then `4`).
   * `UpDirectionLowestFirst`: Sorts an `UP`-committed car's stops ascending (e.g., `2` then `8`).
   * `SingleStopUnchanged`: Leaves a 1-stop queue untouched.
5. **`AssignCallsBatch` (`assign_calls_batch` — 2 tests):**
   * `GroupsSameDirectionCallsOntoOneCar`: Groups Floor 4 `DOWN` and Floor 6 `DOWN` onto one car and assigns Floor 1 `UP` to the other car.
   * `NoCarAvailableLeavesCallUnassigned`: Leaves `assignedCarId = -1` and returns unassigned count `1` when all cars are `OUT_OF_SERVICE`.

## 19. Error-Handling Summary

* **Null Pointer Guards:** Every module function checks pointer parameters (`car`, `out_state`, `out_nextFloor`, `filename`, `logFile`) and immediately returns a negative error code (`-1` or `-2`) if `NULL`.
* **Range & Capacity Validation:** `add_destination()` rejects floors outside `[0, MAX_FLOORS - 1]` (`-3`) and full queues at `MAX_STOPS` (`-2`). `load_config()` clamps configuration values to array limits (`MAX_CARS`, `MAX_FLOORS`, `MAX_CALLS`).
* **Interactive Input Recovery:** `read_int_input()` in `main.c` checks `scanf` return values and range bounds in a loop, flushing invalid characters from `stdin` so non-integer or out-of-range keystrokes never crash the program.
* **File I/O Checks:** `load_config()`, `log_event()`, `run_once()`, and `view_last_report()` check `fopen()` return values and print clear warnings or error messages if files cannot be opened.

## 20. Student Information

* **Student Name:** Vaibhav Inamdar
* **Roll Number:** 612509059
* **Division / Branch:** Instrumentation & Control (Batch 1)

## 21. Known Limitations

* **Single-Pass Sweep Simplification:** `sort_destinations_for_sweep()` assumes all calls assigned to a car in a batch can be served in one monotonic sweep (`UP` lowest-first, `DOWN` highest-first). It is a deliberate batch collective-control simplification rather than a real-time motion simulator.
* **Pre-Dispatch Sensor Evaluation:** Sensor states are evaluated before batch assignment rather than mid-travel between individual floors.

## 22. Future Enhancements

* **Mid-Run Fault Reassignment:** Allow sensor readings to be updated between floor stops during a sweep, dynamically transferring remaining queued stops from a faulted car to healthy in-service cars.
* **Multi-Pass Look-Ahead Scheduling:** Split a car's stop queue into primary and secondary sweeps when an `UP`-committed car is assigned a call below its current starting floor.
* **Live Tick-Based CLI Visualization:** Render an ASCII shaft diagram showing cars moving floor-by-floor in real time.

## 23. GitHub Repository Link

**Repository URL:** [https://github.com/VI1887/Multi-Elevator-Dispatch-Safety-Controller](https://github.com/VI1887/Multi-Elevator-Dispatch-Safety-Controller)