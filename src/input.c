#include <stdlib.h>
#include <string.h>
#include "input.h"

static void trim_newline(char *line) {
    size_t len = strlen(line);
    while (len > 0 && (line[len - 1] == '\n' || line[len - 1] == '\r')) {
        line[len - 1] = '\0';
        len--;
    }
}

int load_config(const char *filename, int *out_numCars, int *out_numFloors,
                 int *out_maxTargets) {
    if (filename == NULL || out_numCars == NULL || out_numFloors == NULL ||
        out_maxTargets == NULL) {
        return -1;
    }

    FILE *fp = fopen(filename, "r");
    if (fp == NULL) {
        return -2;
    }

    char line[MAX_LINE_LENGTH];
    int numCars = -1, numFloors = -1, maxTargets = -1;

    while (fgets(line, sizeof(line), fp) != NULL) {
        trim_newline(line);
        if (line[0] == '\0' || line[0] == '#') {
            continue; /* skip blank lines and comments */
        }
        if (strncmp(line, "numCars", 7) == 0) {
            sscanf(line, "numCars=%d", &numCars);
        } else if (strncmp(line, "numFloors", 9) == 0) {
            sscanf(line, "numFloors=%d", &numFloors);
        } else if (strncmp(line, "maxTargets", 10) == 0) {
            sscanf(line, "maxTargets=%d", &maxTargets);
        }
    }
    fclose(fp);

    if (numCars <= 0 || numFloors <= 0 || maxTargets <= 0) {
        return -3;
    }
    if (numCars > MAX_CARS) {
        numCars = MAX_CARS;
    }
    if (numFloors > MAX_FLOORS) {
        numFloors = MAX_FLOORS;
    }
    if (maxTargets > MAX_CALLS) {
        maxTargets = MAX_CALLS;
    }

    *out_numCars = numCars;
    *out_numFloors = numFloors;
    *out_maxTargets = maxTargets;
    return 0;
}