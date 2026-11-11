#ifndef REPORT_H
#define REPORT_H

#include "common.h"

int log_event(FILE *logFile, const char *description, int carId, int floor);

void generate_summary(FILE *out, const CallRequest calls[], int numCalls,const int floorVisits[], int numFloors,const int overrideCounts[5]);
enum
{
    OV_FIRE_ALARM = 0,
    OV_EMERGENCY_STOP,
    OV_OVERLOAD,
    OV_DOOR_OBSTRUCTED,
    OV_MANUAL_HOLD
};

#endif
