/*
 * Created by Matteo Pompili on 4/24/17.
 * MSc. Physics Student @ La Sapienza
 * */

#ifndef TDC_UTILS_H
#define TDC_UTILS_H

#include <ctime>
#include <functional>

void log_error_and_exit(const char *error_message);

uint64_t abs_diff_64(uint64_t x, uint64_t y);

uint64_t custom_ratio(uint64_t x, uint64_t y);

void u64_vectorize_function(uint64_t* array, uint64_t arraySize, std::function<uint64_t (uint64_t)> func);

void u64_apply_function(uint64_t* array, uint64_t start_index, uint64_t end_index, std::function<uint64_t (uint64_t)> func);

#endif //TDC_UTILS_H
