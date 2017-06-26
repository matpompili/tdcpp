/*
 * Created by Matteo Pompili on 4/22/17.
 * MSc. Physics Student @ La Sapienza
 * */

#include <iostream>
#include <cstring>
#include "TDC_data.h"

/*
 * This is the default constructor. It is called only by derived classes.
 * */
TDC_data::TDC_data() {
}

/*
 * This is the default destructor. It frees the arrays.
 * */
TDC_data::~TDC_data() {
    free(this->timestamp);
    free(this->channel);
}

/*
 * This is an additional constructor. It must be used when you need to read a timestamp binary file.
 * */
void TDC_data::loadFromFile(const char *data_file_path, uint16_t clock, uint16_t box_number) {
//    Open the file
    this->data_file = fopen(data_file_path, "rb");

    if (this->data_file) {
//        If the file is available
        this->size = getFileSize();

        if (this->size > 0) {
//            And it is not empty

            char* read_buffer = (char*) malloc(this->size * TDC_RECORD_SIZE);
            this->timestamp = (uint64_t *) malloc(this->size * sizeof(uint64_t));
            this->channel = (uint16_t *) malloc(this->size * sizeof(uint32_t));

            if( read_buffer == NULL || this->timestamp == NULL || this->channel == NULL) {
                logErrorAndExit("Could not allocate the memory to read a file.");
            }
//            Seek until the end if the header
            fseek(this->data_file, TDC_HEADER_SIZE, SEEK_SET);

//            Read the whole file in the buffer. Faster than reading record by record.
            fread(read_buffer, TDC_RECORD_SIZE, this->size, this->data_file);
//            Close the file, it is not longer needed
            fclose(this->data_file);

            for (int i = 0; i < this->size; i++) {
                memcpy(this->timestamp+i, read_buffer + i*TDC_RECORD_SIZE, TDC_TIMESTAMP_SIZE);
                memcpy(this->channel + i, read_buffer + i*TDC_RECORD_SIZE + TDC_TIMESTAMP_SIZE, TDC_CHANNEL_SIZE);
            }

            free(read_buffer);
        } else {
//            Close the file, it is not longer needed
            fclose(this->data_file);
        }

    } else {
//        The file was not found. Throw an error and exit.
        std::string error_string ("File not found, ");
        error_string.append(data_file_path);
        logErrorAndExit(error_string.c_str());
    }

//    Set the remaining members of the class
    this->clock = clock;
    this->box_number = box_number;
    this->max_channel = 8;
}

/*
 * This method finds the number of events inside the file.
 * */
uint64_t TDC_data::getFileSize() {
    if (this->data_file) {
//        Get starting position
        const int64_t current_position = ftell(this->data_file);
//        Go to EOF and get the position
        fseek(this->data_file, 0, SEEK_END);
        int64_t file_size = ftell(this->data_file);
//        Get back to the beginning
        fseek(this->data_file, current_position, SEEK_SET);
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

/*
 * Getter method for the timestamp array
 * */
uint64_t TDC_data::getTimestamp(uint64_t index) const {
    return *(this->timestamp + index);
}

/*
 * Getter method for the channel array.
 * This one takes into account the box number, it works as follows:
 * The first box has box_number = 1. The channels, as given by the ID800, go from 0 to max_channel-1.
 * They will be returned by this method as 1 to max_channel.
 * If the box_number is higher than 1, 8 * (box_number-1) will be added to the result.
 * It is worth noting that inside the channel array the channels are always stored as going from 0 to max_channel-1.
 * */
uint16_t TDC_data::getChannel(uint64_t index) const {
    return (uint16_t) (*(this->channel + index) + (this->box_number - 1) * 8 + 1);
}

/*
 * Getter method for size. This is the size of both the timestamp array and the channel array.
 * */
uint64_t TDC_data::getSize() const {
    return this->size;
}

/*
 * Getter method for box_number.
 * */
uint16_t TDC_data::getBoxNumber() const {
    return this->box_number;
}

/*
 * This method finds the index at which one second was passed since the first event.
 * It uses binary search.
 * */
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

/*
 * This method returns the number of clock events in the object, as well as an array with the clock events.
 * */
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

/*
 * This method returns the index of the nth trigger.
 * */
uint64_t TDC_data::findNthClock(uint64_t trigger) {
    uint64_t index = 0;
    do {
        if (*(this->channel + index) + 1 == this->clock) trigger--;
        index++;
    } while (trigger > 0);

    return index - 1;
}

/*
 * This method returns true if the event is a trigger, false otherwise.
 * */
bool TDC_data::isClock(uint64_t index) const {
    return (this->channel[index] + 1 == this->clock);
}

/*
 * Getter for the trigger channel.
 * */
uint16_t TDC_data::getClockChannel() const {
    return this->clock;
}

/*
 * Getter for the max_channel
 * */
uint16_t TDC_data::getMaxChannel() const {
    return max_channel;
}

/*
 * This method finds n-fold coincidences in the object, as well as the number of single events on each channel.
 * It writes the results in the two files.
 * Arguments:
 *  - n:                        u16
 *      the ~exact~ number of events that must occur at the same time (modulo coincidence_window).
 *      If more, or less, than n events occur in the coincidence_window, the coincidence will be ignored.
 *
 *  - singles_file_name:        const char pointer
 *      the name of the file in which the single events count will be saved.
 *
 *  - coincidence_file_name:    const char pointer
 *      the name of the file in which the coincidence events will be saved.
 *
 *  - coincidence_window:       u64
 *      the maximum time distance ~in bins~ in which two or more events are considered coincident.
 *      Note: not real time, bins!
 *
 * */
void TDC_data::findNfoldCoincidences(uint16_t n,
                                     const char *singles_file_name,
                                     const char *coincidences_file_name,
                                     uint64_t coincidence_window) {

//    Allocate and set to zero the array for single events.
    uint64_t *singles = (uint64_t *) calloc(this->max_channel, sizeof(uint64_t));
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

    for (uint64_t channel_index = 0; channel_index < this->max_channel; ++channel_index) {
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




