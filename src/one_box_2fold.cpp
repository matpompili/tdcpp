#include <iostream>
#include "TDCpp/TDCpp_data.h"

int main() {
    TDCpp_data *data = new TDCpp_data();
    data->load_from_file("timestamps1.txt", 8, 1);

    data->set_channel_offset("offset.conf");
    data->find_n_fold_coincidences(2, "singles.temp", "coincidences.temp", 30);

    delete data;

    FILE *done_file = fopen("done.task", "w+");
    fprintf(done_file, "Task completed.\n");
    fclose(done_file);

    return 0;
}