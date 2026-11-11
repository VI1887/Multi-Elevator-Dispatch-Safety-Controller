# Phase 1: Project Proposal

## 5.1 Project Title

**Multi-Elevator Dispatch & Safety Controller**

## 5.2 Student Information

| Field | Detail |
|---|---|
| Student Name | Vaibhav Inamdar |
| Roll Number | 612509059 |
| Branch | Instrumentation & Control |
| Batch | 1 |
| Project Title | Multi-Elevator Dispatch & Safety Controller |
| GitHub Repository | https://github.com/VI1887/Multi-Elevator-Dispatch-Safety-Controller |

## 5.3 Engineering Scenario

**Domain:** Transportation / Building Infrastructure Automation

**The real-world situation:**
Most large buildings - offices, apartments, hospitals, malls - don't have just one elevator. They have several, usually grouped together and sharing the same hall call buttons. Whenever someone presses the up or down button, the building has to decide which of these cars should go pick them up. At the same time, each car on its own has to keep handling things like doors, overload sensors, fire alarms, and emergency stops, and none of that can be allowed to take a back seat to "just picking someone up." If two of these things happen together - say, a car is already overloaded when it's needed for a call - the system has to know exactly which one wins, every single time.

**Where the problem occurs:**
Anywhere a building uses more than one elevator sharing the same call buttons. This is common enough that it has its own name in the industry: a "group" or "bank" of elevators.

**Who faces the problem:**
- People using the building, who feel the effects directly when cars are dispatched badly - longer waits, or a car that takes a strange route.
- Facility managers, who are responsible for the elevators running safely and efficiently.
- Maintenance staff, who need to know when and why something went wrong so they can actually fix it, instead of guessing.

**Why a software solution is useful:**
If you just send "whichever car is closest" without thinking about which direction people actually want to go, you end up with cars reversing direction for no good reason, uneven wear between cars, and passengers waiting longer than they should. A proper controller fixes that by grouping requests sensibly - sending one car to handle everyone travelling in the same direction instead of sending two cars to do overlapping work - and by keeping a record of what happened so problems can be traced back to a cause.

## 5.4 Problem Statement

**The existing problem:**
With more than one elevator car, there's no obvious "correct" answer to two questions that come up constantly: which car should answer a given floor request, and in what order should a car visit its stops. Without a clear, fixed procedure for deciding this, you can end up with unsafe behaviour when multiple conditions happen at once, cars assigned in a way that wastes time, and a system where nobody can tell afterward why a certain decision was made.

**Limitations of a naive system:**
- No fixed order of priority between safety conditions and normal call-answering - so it's unclear what should happen if more than one applies at the same time.
- Calls handed to whichever car is physically closest, without checking whether that car is already committed to serving requests in the same direction - which causes two cars to do the work of one, or one car to zig-zag.
- If a car is overloaded, has an open-door fault, or is stopped for an emergency, there's no guarantee it gets taken out of consideration before any dispatch decision is made.
- Nothing is recorded, so there's no way to look back afterward and see how the system performed - which floors are busiest, which safety conditions keep coming up.

**Information the system needs to keep track of:**
- For each car: which floor it's on, which direction it's currently committed to, its operating state, its queued stops, and whether it's in service.
- Each car's sensor readings: emergency stop, fire alarm, door obstruction, load percentage, manual hold.
- A batch of floor requests for the current run: which floor each one came from and which direction was requested.
- A log of what happened: which car was sent where, every stop a car made, and how often each safety condition was triggered.

**What the solution should achieve:**
A C program that collects the current status of every elevator and every pending floor request directly from the person running it, decides each car's safety state, works out which car should serve each request, and then works out the order each car should visit its stops in. At the end of a run it should summarise how the whole batch was handled.

## 5.5 Proposed Solution

**What the application will do:**
The program models a small group of elevator cars serving a fixed number of floors. Everything the simulation needs is entered directly at the keyboard when the program runs - the only file involved is a small configuration file holding fixed limits (number of cars, number of floors, and the maximum number of floor requests allowed per run). When it starts, the program:

1. Asks for each car's current floor.
2. Asks for each car's sensor readings, and immediately works out whether that car is safe to use.
3. Asks how many target-floor requests there are, then asks for each one's floor and the direction the waiting passenger wants to travel.
4. Decides which car should answer each request, grouping requests heading the same direction onto the same car wherever that makes sense, instead of spreading them out car by car.
5. Works out the order each car should visit its assigned stops in, so it sweeps through them in one pass rather than reversing direction partway.
6. Logs everything to a file and prints a summary once the run is done.

**How it will help the user:**
It gives a working, testable example of how a group of elevators should actually make decisions - which car answers which call, and in what order it should move - along with a report that would genuinely be useful to a facility manager trying to understand how a batch of requests was handled.

**Major operations it will support:**
- Reading each car's sensor state and deciding its safety status in a fixed priority order.
- Grouping a batch of floor requests by direction and assigning each one to the best available car.
- Working out the visiting order for each car's stops so it completes one sweep instead of doubling back.
- Writing events and decisions to a log file.
- Producing a summary report once a run finishes.

**What output or reports it will generate:**
- A log showing every stop a car made and any car that was taken out of service.
- A final summary covering which car answered which request (and how far it had to travel to reach it), how many times each floor was visited, and how often each safety condition was triggered.

## 5.6 Target Users

| User | How they use the system |
|---|---|
| Facility/Building Manager | Looks at the summary report to judge how well a batch of requests was handled, and to plan maintenance. |
| Maintenance Technician | Uses the safety-override counts to spot which car or which condition needs attention. |
| Building Occupants (indirect) | Don't use the system directly, but are the ones who benefit from sensible dispatch and safe operation. |

## 5.7 Project Scope

**In-scope features:**
- A group of elevator cars, with the number of cars and floors set in a configuration file.
- All simulation input (car positions, sensor readings, floor requests) entered directly by the person running the program.
- Safety priority logic for each car (fire alarm, emergency stop, overload, door obstruction, manual hold).
- Logic to group floor requests by direction and assign each one to the best car, rather than treating every request in isolation.
- A sweep-ordering rule so a car visits all its assigned stops in one direction before it would need to reverse.
- Logging events and state changes to a file.
- A summary report once a run ends (dispatch outcome per request, floor traffic, safety override counts).

**Out-of-scope features:**
- Any real hardware or sensor integration - every input is typed in by a person standing in for real sensors and passengers.
- A graphical or web interface - this stays a console program.
- Real network communication between devices or buildings.
- Actual motor or actuator control - the project is about the decision-making logic, not controlling physical hardware.

## 5.8 Functional Requirements

| ID | Requirement |
|---|---|
| FR-01 | The system shall load a configurable number of elevator cars, building floors, and the maximum number of target-floor requests per run from a configuration file. |
| FR-02 | The system shall ask for and accept each car's current floor directly from the person running the program. |
| FR-03 | The system shall ask for and accept each car's sensor readings (emergency stop, fire alarm, overload, door obstruction, manual hold) directly from the person running the program. |
| FR-04 | The system shall decide each car's safety state from its sensor readings, in a fixed priority order, before that car is considered for dispatch. |
| FR-05 | The system shall accept a configurable number of target-floor requests, each with a floor and a requested direction. |
| FR-06 | The system shall assign each target-floor request to the most suitable in-service car, grouping requests in the same direction onto the same car where that is the better choice. |
| FR-07 | The system shall work out the order in which a car visits its assigned stops, so it completes a sweep in one direction before reversing. |
| FR-08 | The system shall log every processed decision and resulting stop to a file. |
| FR-09 | The system shall generate a summary report at the end of a run, covering the dispatch outcome of every request, how many times each floor was visited, and how often each safety condition occurred. |
| FR-10 | The system shall validate all input (floor numbers in range, no negative values, valid menu choices) and reject invalid input without crashing. |

## 5.9 Non-Functional Requirements

| ID | Requirement |
|---|---|
| NFR-01 | The system shall be written in modular C, with each major responsibility - input, safety, dispatch, scheduling, reporting - kept in its own source file. |
| NFR-02 | `main.c` shall only handle collecting input and calling the other modules in order, with no safety, dispatch, or scheduling decision made directly inside it. |
| NFR-03 | The core decision functions (safety checks, dispatch scoring, stop ordering) shall take clear inputs and return clear outputs, so they can be tested on their own. |
| NFR-04 | The code shall follow consistent naming and formatting throughout, with function headers explained in comments. |
| NFR-05 | The code shall compile without warnings using the project's Makefile. |
| NFR-06 | At least three core functions shall have unit tests covering normal cases, invalid cases, and edge cases, written using Google Test. |

## 5.10 Initial Modules

| Module | Responsibility |
|---|---|
| `input` | Reads the configuration file (number of cars, floors, request limit). |
| `safety` | Decides a car's safety state from its sensor flags, in a fixed priority order. |
| `dispatch` | Assigns a batch of floor requests to the best cars, grouping same-direction requests where sensible. |
| `scheduler` | Maintains each car's stop queue and works out the order it should be visited in. |
| `report` | Logs events as they happen and builds the end-of-run summary. |
| `main` | Collects all input from the person running the program and calls the other modules in order. |

## 5.11 Initial Architecture Diagram

The system is organised into the following layers, from top to bottom:

- **User** - types in each car's position, each car's sensor readings, and every target-floor request.
- **Application interface (`main.c`)** - asks for that input and controls the overall flow of the program.
- **Application modules** - `safety` decides each car's state; `dispatch` decides which car answers each request; `scheduler` decides the order each car visits its stops in.
- **File storage and reports (`report`)** - writes the event log as the program runs, and produces the summary report at the end.

So the flow is: the user enters a car's status or a floor request → `main.c` passes it to `safety` or collects it for `dispatch` → `dispatch`'s assignment is handed to `scheduler` → `scheduler`'s stop order is logged and eventually summarised by `report`.

## 5.12 High-Level Workflow

1. Program starts and loads the number of cars, floors, and request limit from the configuration file.
2. The program asks for each car's current floor.
3. The program asks for each car's sensor readings and works out that car's safety state right away.
4. The program asks how many target-floor requests there are, then asks for each one's floor and direction.
5. The dispatch module assigns every request to a car, grouping same-direction requests where that's the better choice.
6. For each car with stops queued, the scheduler module works out the visiting order, and the program steps through that order, printing and logging every stop.
7. The program prints the final summary report and exits (or returns to the menu to run again or view the last report).

## 5.13 Planned C Concepts

| Concept | Why it's needed |
|---|---|
| Functions | Each module's logic is broken into small, single-purpose functions with clear inputs and outputs. |
| Structures | `ElevatorCar` and `CallRequest` need to group related data together. |
| Enumerations | Used for things like car state and direction, instead of plain numbers, so the code stays readable. |
| Arrays | Used to hold the list of cars, each car's queued stops, and the batch of floor requests. |
| Strings | Used for reading the configuration file and for printing/writing log and report output. |
| Pointers | Used for output parameters (so a function can return more than one value) and for passing structures to functions without copying them. |
| Constants | Used for fixed limits like the maximum number of floors or queued stops, instead of unexplained numbers in the code. |
| File handling | Used to read the configuration file and to write the log and summary report. |
| Error/status codes | Functions return a status code (success or a specific error) instead of failing silently. |
