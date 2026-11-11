#include <gtest/gtest.h>

extern "C"
{
    #include "common.h"
    #include "safety.h"
    #include "scheduler.h"
    #include "dispatch.h"
}

// Test Case 1: Normal case - no flags set, car stays IDLE
TEST(SafetyState, NormalNoFlags)
{
    CarState out = OUT_OF_SERVICE;
    EXPECT_EQ(0, decide_car_safety_state(false, false, false, 0, false, IDLE, &out));
    EXPECT_EQ(IDLE, out);
}

// Test Case 2: Priority order - fire alarm beats everything else
TEST(SafetyState, FireAlarmHasTopPriority)
{
    CarState out;
    // fire alarm true, overload true at the same time -> fire alarm wins
    EXPECT_EQ(0, decide_car_safety_state(false, true, false, 100, false, IDLE, &out));
    EXPECT_EQ(OUT_OF_SERVICE, out);

    // fire alarm true, emergency stop true at the same time -> fire alarm wins
    EXPECT_EQ(0, decide_car_safety_state(true, true, false, 0, false, IDLE, &out));
    EXPECT_EQ(OUT_OF_SERVICE, out);
}

// Test Case 3: Overload (load_percent >= 100) with no higher-priority flag -> DOOR_OPEN
TEST(SafetyState, OverloadOpensDoor)
{
    CarState out;
    EXPECT_EQ(0, decide_car_safety_state(false, false, false, 100, false, IDLE, &out));
    EXPECT_EQ(DOOR_OPEN, out);
}

// Test Case 4: Recovery - clears DOOR_OPEN/OUT_OF_SERVICE back to IDLE once flags clear
TEST(SafetyState, RecoversToIdleOnceFlagsClear)
{
    CarState out;
    EXPECT_EQ(0, decide_car_safety_state(false, false, false, 0, false, DOOR_OPEN, &out));
    EXPECT_EQ(IDLE, out);

    EXPECT_EQ(0, decide_car_safety_state(false, false, false, 0, false, OUT_OF_SERVICE, &out));
    EXPECT_EQ(IDLE, out);
}

// Null pointer check: NULL out_state must return -1
TEST(SafetyState, NullOutStateRejected)
{
    EXPECT_EQ(-1, decide_car_safety_state(false, false, false, 0, false, IDLE, nullptr));
}

// Test Case 5: Normal add - floor is queued and count increases
TEST(AddDestination, NormalAdd)
{
    ElevatorCar car = {0};
    EXPECT_EQ(0, add_destination(&car, 5));
    EXPECT_EQ(1, car.numDestinations);
    EXPECT_EQ(5, car.destinationFloors[0]);
}

// Test Case 6: Invalid floor (out of range) is rejected
TEST(AddDestination, OutOfRangeFloorRejected)
{
    ElevatorCar car = {0};
    EXPECT_EQ(-3, add_destination(&car, MAX_FLOORS + 5));
    EXPECT_EQ(0, car.numDestinations);
}

// Edge case: queue already full
TEST(AddDestination, FullQueueRejected)
{
    ElevatorCar car = {0};
    car.numDestinations = MAX_STOPS;
    EXPECT_EQ(-2, add_destination(&car, 1));
}

// Null pointer check: NULL car must return -1
TEST(AddDestination, NullCarRejected)
{
    EXPECT_EQ(-1, add_destination(nullptr, 1));
}

// Test Case 7: Normal case - nearest stop in the car's current direction is chosen
TEST(GetNextStop, NearestInCurrentDirection)
{
    ElevatorCar car = {0};
    car.currentFloor = 2;
    car.direction = UP;
    car.destinationFloors[0] = 5;
    car.destinationFloors[1] = 4;
    car.numDestinations = 2;

    int nextFloor = -1;
    EXPECT_EQ(0, get_next_stop(&car, &nextFloor));
    EXPECT_EQ(4, nextFloor);
}

// Edge case: nothing left ahead in the current direction -> car must reverse
TEST(GetNextStop, ReversesWhenNothingAhead)
{
    ElevatorCar car = {0};
    car.currentFloor = 8;
    car.direction = UP;
    car.destinationFloors[0] = 3; // behind the car
    car.numDestinations = 1;

    int nextFloor = -1;
    EXPECT_EQ(0, get_next_stop(&car, &nextFloor));
    EXPECT_EQ(3, nextFloor);
}

// Edge case: empty queue returns an error, not a crash
TEST(GetNextStop, EmptyQueueReturnsError)
{
    ElevatorCar car = {0};
    int nextFloor = -1;
    EXPECT_EQ(-1, get_next_stop(&car, &nextFloor));
}

// Null pointer check
TEST(GetNextStop, NullCarRejected)
{
    int nextFloor = -1;
    EXPECT_EQ(-2, get_next_stop(nullptr, &nextFloor));
}

// Test Case 8: a DOWN-committed car visits its stops highest floor first
TEST(SortForSweep, DownDirectionHighestFirst)
{
    ElevatorCar car = {0};
    car.currentFloor = 0;
    car.direction = DOWN;
    car.destinationFloors[0] = 4;
    car.destinationFloors[1] = 6;
    car.numDestinations = 2;

    sort_destinations_for_sweep(&car);
    EXPECT_EQ(6, car.destinationFloors[0]);
    EXPECT_EQ(4, car.destinationFloors[1]);
}

// Test Case 9: an UP-committed car visits its stops lowest floor first
TEST(SortForSweep, UpDirectionLowestFirst)
{
    ElevatorCar car = {0};
    car.currentFloor = 0;
    car.direction = UP;
    car.destinationFloors[0] = 8;
    car.destinationFloors[1] = 2;
    car.numDestinations = 2;

    sort_destinations_for_sweep(&car);
    EXPECT_EQ(2, car.destinationFloors[0]);
    EXPECT_EQ(8, car.destinationFloors[1]);
}

// Edge case: a single queued stop is left unchanged
TEST(SortForSweep, SingleStopUnchanged)
{
    ElevatorCar car = {0};
    car.direction = DOWN;
    car.destinationFloors[0] = 5;
    car.numDestinations = 1;

    sort_destinations_for_sweep(&car);
    EXPECT_EQ(5, car.destinationFloors[0]);
}

// Test Case 10: matches the worked example - floor 4 DOWN and floor 6 DOWN
// should group onto the same car; floor 1 UP should go to the other car.
TEST(AssignCallsBatch, GroupsSameDirectionCallsOntoOneCar)
{
    ElevatorCar cars[2] = {};
    for (int i = 0; i < 2; i++)
    {
        cars[i].carId = i;
        cars[i].currentFloor = 0;
        cars[i].direction = NONE;
        cars[i].state = IDLE;
        cars[i].numDestinations = 0;
        cars[i].inService = true;
    }

    CallRequest calls[3] = {
        {4, DOWN, -1, -1},
        {6, DOWN, -1, -1},
        {1, UP,   -1, -1}
    };

    EXPECT_EQ(0, assign_calls_batch(cars, 2, calls, 3));
    EXPECT_EQ(calls[0].assignedCarId, calls[1].assignedCarId);
    EXPECT_NE(calls[2].assignedCarId, calls[0].assignedCarId);
}

// Edge case: no in-service car available -> call is left unassigned, not crashed
TEST(AssignCallsBatch, NoCarAvailableLeavesCallUnassigned)
{
    ElevatorCar cars[1] = {};
    cars[0].carId = 0;
    cars[0].currentFloor = 0;
    cars[0].direction = NONE;
    cars[0].state = OUT_OF_SERVICE;
    cars[0].inService = false;
    cars[0].numDestinations = 0;

    CallRequest calls[1] = {{3, UP, -1, -1}};

    EXPECT_EQ(1, assign_calls_batch(cars, 1, calls, 1));
    EXPECT_EQ(-1, calls[0].assignedCarId);
}
