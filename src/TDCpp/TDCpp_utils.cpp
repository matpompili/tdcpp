/*
 * Created by Matteo Pompili on 4/27/17.
 * MSc. Physics Student @ La Sapienza
 * */

#include <iostream>
#include <thread>
#include <cstring>
#include "TDCpp_utils.h"

#define NUM_THREADS 8

void log_error_and_exit(const char *error_message) {
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

uint64_t abs_diff_64(uint64_t x, uint64_t y) {
    if (x > y) return x - y;
    return y - x;
}

uint64_t custom_ratio(uint64_t x, uint64_t y) {
    if (x == 0 || y == 0) return 0;
    if (x > y) return x/y;
    return y/x;
}

void u64_vectorize_function(uint64_t *array, uint64_t arraySize, std::function<uint64_t (uint64_t)> func) {
    std::thread threads[NUM_THREADS];
    uint64_t blk_size = arraySize/NUM_THREADS;
    for (uint16_t i = 0; i < NUM_THREADS - 1; ++i) {
        threads[i] = std::thread(u64_apply_function, array, i * blk_size, (i+1) * blk_size, func);
    }
    threads[NUM_THREADS-1] = std::thread(u64_apply_function, array, (NUM_THREADS-1) * blk_size, arraySize, func);

    for (uint16_t i = 0; i < NUM_THREADS; ++i) {
        threads[i].join();
    }
}

void u64_apply_function(uint64_t *array,
                        uint64_t start_index, uint64_t end_index, std::function<uint64_t(uint64_t)> func) {

    uint64_t *local_array = (uint64_t *) malloc((end_index - start_index) * sizeof(uint64_t));

    for(uint64_t i = 0; i < end_index - start_index;  ++i) {
        local_array[i] = func(array[i + start_index]);
    }

    memcpy(array + start_index, local_array, (end_index - start_index) * sizeof(uint64_t));
}
