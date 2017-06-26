/*
 * Created by Matteo Pompili on 4/27/17.
 * MSc. Physics Student @ La Sapienza
 * */

#include <iostream>
#include "TDC_utils.h"

void logErrorAndExit(const char *error_message) {
    std::cerr << "Fatal error: " << error_message << std::endl;
    FILE* logFile = fopen("error.log", "a+");
    if (logFile) {
//        Get the current time. https://stackoverflow.com/a/1531191/1365456
        time_t current_time;
        struct tm * time_info;
        char timeString[32];

        time(&current_time);
        time_info = localtime(&current_time);

        strftime(timeString, sizeof(timeString), "%c", time_info);
//        Print error and timestamp
        fprintf(logFile, "%s::Fatal error::%s\n", timeString, error_message);
    }

    exit(EXIT_FAILURE);
}

uint64_t customRatio(uint64_t x, uint64_t y) {
    if ( x == 0 || y == 0 ) return 0;
    if (x > y) return x/y;
    return y/x;
}
