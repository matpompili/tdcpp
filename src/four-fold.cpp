#include <iostream>
#include <thread>
#include <future>
#include "TDCpp/TDCpp_data.h"
#include "TDCpp/TDCpp_merger.h"

//#define TIME_PROGRAM
#ifdef TIME_PROGRAM
    #include <ctime>
#endif

int main() {
#ifdef TIME_PROGRAM
    clock_t start_clock = clock();
#endif

    TDCpp_data *first_file = new TDCpp_data();
    TDCpp_data *second_file = new TDCpp_data();
    TDCpp_data *third_file = new TDCpp_data();

    std::thread first_thread(&TDCpp_data::load_from_file, first_file, "timestamps1.txt", 8, 1);
    std::thread second_thread(&TDCpp_data::load_from_file, second_file, "timestamps2.txt", 8, 2);
    std::thread third_thread(&TDCpp_data::load_from_file, third_file, "timestamps3.txt", 8, 3);

    first_thread.join();
    second_thread.join();
    third_thread.join();

    TDCpp_merger *first_plus_second = new TDCpp_merger(first_file, second_file);
    delete first_file;
    delete second_file;

    TDCpp_merger *all_together = new TDCpp_merger(first_plus_second, third_file);
    delete first_plus_second;
    delete third_file;

    all_together->find_n_fold_coincidences(4, "singles.temp", "coincidences.temp", 100, true);

    delete all_together;

    FILE* done_file = fopen("done.task", "w+");
    fprintf(done_file, "Task completed.\n");
    fclose(done_file);

#ifdef TIME_PROGRAM
    std::cout << ((double) clock() - start_clock) / CLOCKS_PER_SEC << std::endl;
#endif

    return 0;
}