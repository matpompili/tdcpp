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
        fprintf(logFile, "Fatal error: %s\n", error_message);
    }

    exit(EXIT_FAILURE);
}
