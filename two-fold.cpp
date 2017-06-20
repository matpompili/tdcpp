/*
 * Created by Matteo Pompili on 4/22/17.
 * MSc. Physics Student @ La Sapienza
 * */

#include <iostream>
#include <thread>
#include <future>
#include "TDC_data.h"
#include "TDC_merger.h"

#define TIME_PROGRAM
#ifdef TIME_PROGRAM
#include <ctime>
#endif

int main() {
#ifdef TIME_PROGRAM
    clock_t start_clock = clock();
#endif

    TDC_data *first_file = new TDC_data();
    TDC_data *second_file = new TDC_data();
    TDC_data *third_file = new TDC_data();

    std::thread first_thread(&TDC_data::loadFromFile, first_file, "timestamps1.txt", 8, 1);
    std::thread second_thread(&TDC_data::loadFromFile, second_file, "timestamps2.txt", 8, 2);
    std::thread third_thread(&TDC_data::loadFromFile, third_file, "timestamps3.txt", 8, 3);

    first_thread.join();
    second_thread.join();
    third_thread.join();

    TDC_merger *first_plus_second = new TDC_merger(first_file, second_file);
    delete first_file;
    delete second_file;

    TDC_merger *all_together = new TDC_merger(first_plus_second, third_file);
    delete first_plus_second;
    delete third_file;

    all_together->findNfoldCoincidences(2, "singles.temp", "coincidences.temp", 160);

    delete all_together;

    FILE* done_file = fopen("done.task", "w+");
    fprintf(done_file, "Task completed.\n");
    fclose(done_file);

#ifdef TIME_PROGRAM
    std::cout << ((double) clock() - start_clock) / CLOCKS_PER_SEC << std::endl;
#endif

    return 0;
}