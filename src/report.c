#include "report.h"

int log_event(FILE *logFile, const char *description, int carId, int floor) {
    if (logFile == NULL || description == NULL) {
        return -1;
    }

    int written = fprintf(logFile, "car=%d floor=%d : %s\n", carId, floor, description);
    if (written < 0) {
        return -2;
    }
    fflush(logFile);
    return 0;
}

static const char *override_label(int index) {
    switch (index) {
        case OV_FIRE_ALARM:       return "fire alarm";
        case OV_EMERGENCY_STOP:   return "emergency stop";
        case OV_OVERLOAD:         return "overload";
        case OV_DOOR_OBSTRUCTED:  return "door obstructed";
        case OV_MANUAL_HOLD:      return "manual hold";
        default:                  return "unknown";
    }
}

void generate_summary(FILE *out, const CallRequest calls[], int numCalls,
                       const int floorVisits[], int numFloors,
                       const int overrideCounts[5]) {
    if (out == NULL) {
        out = stdout;
    }

    fprintf(out, "===== Simulation Summary =====\n");

    if (numCalls == 0 || calls == NULL) {
        fprintf(out, "No target-floor requests were entered.\n");
    } else {
        fprintf(out, "Target-floor requests and dispatch outcome:\n");
        for (int i = 0; i < numCalls; i++) {
            const char *dirWord = (calls[i].direction == UP) ? "UP" : "DOWN";
            if (calls[i].assignedCarId == -1) {
                fprintf(out, "  Floor %2d (%s) : NOT assigned - no elevator available\n",
                        calls[i].callFloor, dirWord);
            } else {
                fprintf(out, "  Floor %2d (%s) : Elevator %d (travelled %d floor(s) to reach it)\n",
                        calls[i].callFloor, dirWord, calls[i].assignedCarId,
                        calls[i].distanceAtAssignment);
            }
        }
    }

    if (floorVisits != NULL && numFloors > 0) {
        fprintf(out, "\nFloor traffic:\n");
        for (int f = 0; f < numFloors; f++) {
            if (floorVisits[f] > 0) {
                fprintf(out, "  Floor %2d : %d visit(s)\n", f, floorVisits[f]);
            }
        }
    }

    if (overrideCounts != NULL) {
        fprintf(out, "\nSafety overrides triggered:\n");
        bool any = false;
        for (int i = 0; i < 5; i++) {
            if (overrideCounts[i] > 0) {
                fprintf(out, "  %-16s : %d time(s)\n", override_label(i), overrideCounts[i]);
                any = true;
            }
        }
        if (!any) {
            fprintf(out, "  none\n");
        }
    }

    fprintf(out, "===============================\n");
}
