/*
 * Created by Matteo Pompili on 4/22/17.
 * MSc. Physics Student @ La Sapienza
 * */

#include <iostream>
#include <cstring>
#include "TDC_data.h"

TDC_data::TDC_data() {
}

TDC_data::~TDC_data() {
    free(this->timestamp);
    free(this->channel);
}

void TDC_data::loadFromFile(const char *data_file_path, uint16_t clock, uint16_t box_number) {
    FILE *data_file = fopen(data_file_path, "rb");
    ///Open the file

    if (data_file) {
        ///If the file is available
        this->size = getFileSize(data_file);

        if (this->size > 0) {
            ///And it is not empty

            char* read_buffer = (char*) malloc(this->size * TDC_RECORD_SIZE);
            this->timestamp = (uint64_t *) malloc(this->size * sizeof(uint64_t));
            this->channel = (uint16_t *) malloc(this->size * sizeof(uint32_t));

            if( read_buffer == NULL || this->timestamp == NULL || this->channel == NULL) {
                logErrorAndExit("Could not allocate the memory to read a file.");
            }

            fseek(data_file, TDC_HEADER_SIZE, SEEK_SET);
            ///Seek until the end if the header

            fread(read_buffer, TDC_RECORD_SIZE, this->size, data_file);
            ///Read the whole file in the buffer. Faster than reading record by record.

            fclose(data_file);
            ///Close the file, it is not longer needed

            for (int i = 0; i < this->size; i++) {
                memcpy(this->timestamp+i, read_buffer + i*TDC_RECORD_SIZE, TDC_TIMESTAMP_SIZE);
                memcpy(this->channel + i, read_buffer + i*TDC_RECORD_SIZE + TDC_TIMESTAMP_SIZE, TDC_CHANNEL_SIZE);
            }

            free(read_buffer);
        } else {
            fclose(data_file);
            ///Close the file, it is not longer needed
        }

    } else {
        std::string error_string ("File not found, ");
        error_string.append(data_file_path);
        logErrorAndExit(error_string.c_str());
        ///The file was not found. Throw an error and exit.
    }

    /**
     * Set the remaining members of the class.
     */
    this->clock = clock;
    this->box_number = box_number;
    this->num_channels = 8;
    this->offset = (int16_t *) calloc(this->num_channels, sizeof(int16_t));
}

uint64_t TDC_data::getFileSize(FILE *data_file) {
    if (data_file) {
        const int64_t current_position = ftell(data_file);
        ///Get starting position

        fseek(data_file, 0, SEEK_END);
        ///Go to EOF and get the position

        int64_t file_size = ftell(data_file);
        ///Get back to the beginning

        fseek(data_file, current_position, SEEK_SET);
//        If the file is big enough, i.e. at least the header and one record
        if (file_size > TDC_HEADER_SIZE + TDC_RECORD_SIZE) {
//            Get the number of records inside the file
            file_size = (file_size - TDC_HEADER_SIZE) / TDC_RECORD_SIZE;
        } else {
//            Otherwise just say no events are available
            file_size = 0;
        }
        return (uint64_t) file_size;
    } else {
//        The pointer is null, throw an error and exit.
        std::string error_string ("Inside getFileSize the file pointer is null.");
        logErrorAndExit(error_string.c_str());
    }
}

uint64_t TDC_data::getTimestamp(uint64_t index) const {
    return *(this->timestamp + index);
}

uint16_t TDC_data::getChannel(uint64_t index) const {
    return (uint16_t) (*(this->channel + index) + (this->box_number - 1) * 8 + 1);
}

uint64_t TDC_data::findOneSecondIndex() {
    uint64_t start_index = 0;
    uint64_t end_index = this->size - 1;
    uint64_t index;
//    When start_index and end_index are just one position apart the search is completed.
    while (end_index - start_index > 1) {
        index = (end_index + start_index) / 2;
        if (this->timestamp[index] > TDC_ONE_SEC_BINS + this->timestamp[0]) {
            end_index = index;
        } else if (this->timestamp[index] < TDC_ONE_SEC_BINS + this->timestamp[0]) {
            start_index = index;
        } else {
            end_index = index;
            start_index = index;
        }
    }
//    Always return the lesser of the two, arbitrary.
    return start_index;
}

uint64_t TDC_data::getClocks(uint64_t *destination_array) {
    uint64_t index = 0;
    for (int i = 0; i < this->size; ++i) {
//        If this event is a clock
        if (*(this->channel + i) + 1 == this->clock) {
//            Add it to the destination_array
            *(destination_array + index) = *(this->timestamp + i);
            index++;
        }
    }

    return index;
}

uint64_t TDC_data::findNthClock(uint64_t n) {
    uint64_t index = 0;
    do {
        if (*(this->channel + index) + 1 == this->clock) n--;
        index++;
    } while (n > 0);

    return index - 1;
}

bool TDC_data::isClock(uint64_t index) const {
    return (this->channel[index] + 1 == this->clock);
}


void TDC_data::findNfoldCoincidences(uint16_t n,
                                     const char *singles_file_name,
                                     const char *coincidences_file_name,
                                     uint64_t coincidence_window) {

//    Allocate and set to zero the array for single events.
    uint64_t *singles = (uint64_t *) calloc(this->num_channels, sizeof(uint64_t));
//    coincidence_channel is a pointer to an n-size array that is needed to keep
//    track of the events inside a coincidence-window.
    uint16_t *coincidence_channel = (uint16_t *) calloc(n, sizeof(uint16_t));
//    A map that will hold the count of each possible coincidence.
    std::map<std::string, uint64_t> coincidences_map;

    uint16_t coincidence_channel_index = 0;
    uint64_t coincidence_window_start = this->timestamp[0];

    singles[this->channel[0]] += 1;
    coincidence_channel[0] = this->channel[0];
    coincidence_channel_index++;

    bool is_coincidence_still_good = true;
    bool is_channel_acceptable;
    std::string coincidence_key;

//    While there are events
    for (uint64_t i = 1; i < this->size; ++i) {
//        Increase the singles count
        singles[this->channel[i]] += 1;

//        If the event is in the coincidence window
        if (this->timestamp[i] - coincidence_window_start <= coincidence_window) {
//            If there have not been already too much events in this window
            if (coincidence_channel_index < n) {
//                Check if an event with the same channel as already been detected in this window
                is_channel_acceptable = true;
                for (uint16_t j = 0; j < coincidence_channel_index; ++j) {
                    if (this->channel[i] == coincidence_channel[j]) {
                        is_channel_acceptable = false;
                        break;
                    }
                }

//                Add the channel to the coincidence
                if (is_channel_acceptable) {
                    coincidence_channel[coincidence_channel_index] = this->channel[i];
                    coincidence_channel_index++;
                }
            } else {
//                Mark the coincidence as not usable, too many events.
                is_coincidence_still_good = false;
            }
        } else {
//            Save the last coincidence, if there is one
            if (is_coincidence_still_good && (coincidence_channel_index == n)) {
                coincidence_key = "";
//                Create the key for this coincidence
                for (uint16_t j = 0; j < n; ++j) {
                    coincidence_key.append(std::to_string(coincidence_channel[j] + 1));
                    coincidence_key.append("_");
                }
//                Remove the last underscore
                coincidence_key.pop_back();
//                Increase the count
                coincidences_map[coincidence_key] += 1;
            }

//            Start a new window
            coincidence_window_start = this->timestamp[i];
            coincidence_channel[0] = this->channel[i];
            coincidence_channel_index = 1;
            for (uint16_t j = 1; j < n; ++j) {
                coincidence_channel[j] = 0;
            }
            is_coincidence_still_good = true;
        }

    }


//    Save the singles
    FILE *singles_file = fopen(singles_file_name, "w");

    for (uint64_t channel_index = 0; channel_index < this->num_channels; ++channel_index) {
        if (singles[channel_index] != 0) {
            fprintf(singles_file, "%" PRIu64 "\t%" PRIu64 "\n", channel_index + 1, singles[channel_index]);
        }
    }
    free(singles);
    fclose(singles_file);

//    Save the coincidences
    FILE *coincidences_file = fopen(coincidences_file_name, "w");

    for (auto const &map_entry : coincidences_map) {
        fprintf(coincidences_file, "%s\t%" PRIu64 "\n", map_entry.first.c_str(), map_entry.second);
    }
    free(coincidence_channel);
    fclose(coincidences_file);
}

/*
 * This method print to file the timestamps and relative channels of the object.
 * Arguments:
 *  - *output_file_path:    const char pointer
 *      the name of the output file.
 * */
void TDC_data::printDataToFile(const char *output_file_path) {
    FILE* output_file = fopen(output_file_path, "w");
    if (output_file) {
        for (uint64_t i = 0; i < this->size; ++i) {
            fprintf(output_file, "%" PRIu64 " %" PRIu16 "\n", this->timestamp[i], this->getChannel(i));
        }
    } else {
        std::string error_string ("Can't write to  ");
        error_string.append(output_file_path);
        logErrorAndExit(error_string.c_str());
    }

    fclose(output_file);
}

void TDC_data::setChannelOffset(const char *offset_file_path) {
    FILE* offset_file = fopen(offset_file_path, "r");
    int16_t min_offset = 0;

    if (offset_file) {
        for (uint16_t i = 0; i < this->num_channels; ++i) {
            fscanf(offset_file, "%" PRId16 "", this->offset + i);
            if (min_offset > this->offset[i]) min_offset = this->offset[i];
        }

        this->timestamp[0] += -min_offset + this->offset[this->channel[0]];
        uint64_t sorting_timestamp = this->timestamp[0];
        uint16_t sorting_channel = this->channel[0];
        uint64_t j;


        for (uint64_t i = 1; i < this->size; ++i) {
            this->timestamp[i] += -min_offset + this->offset[this->channel[i]];
            sorting_timestamp = this->timestamp[i];
            j = i -1;
            while ( (j >= 0) && (this->timestamp[j] > sorting_timestamp)) {
                this->timestamp[j+1] = this->timestamp[j];
                this->channel[j+1] = this->channel[j];
                j--;
                this->timestamp[j+1] = sorting_timestamp;
                this->channel[j+1] = sorting_channel;
            }
        }

    } else {
        std::string error_string ("Can't read offset file  ");
        error_string.append(offset_file_path);
        logErrorAndExit(error_string.c_str());
    }

    fclose(offset_file);
}
