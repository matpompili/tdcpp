/*
 * Created by Matteo Pompili on 4/24/17.
 * MSc. Physics Student @ La Sapienza
 * */

#include <iostream>
#include "TDC_merger.h"


/*
 * This is the default constructor.
 *
 * Arguments:
 *  - *first_data, *second_data:      TDC_data pointer
 *      the two TDC_data objects that are going to be merged.
 *
 * */
TDC_merger::TDC_merger(TDC_data *first_data, TDC_data *second_data) {

//    Save a reference to the object
    this->first_data = first_data;
    this->second_data = second_data;

//    This is needed to ensure channels are named correctly.
    this->box_number = 1;
    this->clock = this->first_data->getClockChannel();
    this->num_channels = this->first_data->getChannelsNumber() + this->second_data->getChannelsNumber();

    this->offset = (int16_t *) calloc(this->num_channels, sizeof(int16_t));

//    Find the matching index between the two objects. Also find which object is delayed.
    this->findMatch(200, 20);
    this->merge(100);
}

/*
 * This is the default destructor. It frees the arrays and deletes the references.
 * */
TDC_merger::~TDC_merger() {
    this->first_data = nullptr;
    this->second_data = nullptr;

    free(this->first_clocks);
    free(this->second_clocks);
}

/*
 * This method finds the first common clock event in the two TDC_data objects.
 * It consider a `time_depth` 
 * Arguments:
 *  - max_shift:    u64
 *      Max offset to consider. This depend on both the rate of the clock and the starting delay of the boxes.
 *
 *  - time_depth:   u64
 *      Size of the timestamps subarrays to confront.
 * */
void TDC_merger::findMatch(uint64_t max_shift, uint64_t time_depth) {
//    Allocate the arrays that will contains the clock events of the two objects.
    this->first_clocks = (uint64_t *) malloc(this->first_data->getSize() * sizeof(uint64_t));
    this->second_clocks = (uint64_t *) malloc(this->second_data->getSize() * sizeof(uint64_t));
//    Get the clock events, as well as their count.
    this->num_first_clocks = this->first_data->getClocks(this->first_clocks);
    this->num_second_clocks = this->second_data->getClocks(this->second_clocks);

//    Allocate the arrays for the time differences between clock events.
    uint64_t *first_clock_deltas = (uint64_t *) malloc((this->num_first_clocks - 1) * sizeof(uint64_t));
    uint64_t *second_clock_deltas = (uint64_t *) malloc((this->num_second_clocks - 1) * sizeof(uint64_t));

//    Populate the delta arrays
    for (uint64_t i = 0; i < this->num_first_clocks - 1; ++i) {
        first_clock_deltas[i] = this->first_clocks[i + 1] - this->first_clocks[i];
    }

    for (uint64_t i = 0; i < this->num_second_clocks - 1; ++i) {
        second_clock_deltas[i] = this->second_clocks[i + 1] - this->second_clocks[i];
    }

//    Find what is the maximum possible shift between the objects, given the time depth.
    if (this->num_first_clocks - time_depth < max_shift) max_shift = this->num_first_clocks - time_depth;
    if (this->num_second_clocks - time_depth < max_shift) max_shift = this->num_second_clocks - time_depth;

    uint64_t distance_forward, distance_backward;
    uint64_t min_distance_forward, min_distance_forward_position;
    uint64_t min_distance_backward, min_distance_backward_position;

//    Evaluate the time differences.
    for (uint64_t i = 0; i < max_shift; ++i) {
        distance_backward = distance_forward = 0;
        for (uint64_t j = 0; j < time_depth; ++j) {
//            Here `i` is the delay between the objects. `j` is the time index.
            distance_forward += abs_diff_64(first_clock_deltas[j], second_clock_deltas[i + j]);
            distance_backward += abs_diff_64(first_clock_deltas[i + j], second_clock_deltas[j]);
        }

//        Need just on the first step.
        if (i == 0) {
            min_distance_forward = distance_forward;
            min_distance_forward_position = 0;
            min_distance_backward = distance_backward;
            min_distance_backward_position = 0;
        } else {
            if (distance_forward < min_distance_forward) {
                min_distance_forward = distance_forward;
                min_distance_forward_position = i;
            }

            if (distance_backward < min_distance_backward) {
                min_distance_backward = distance_backward;
                min_distance_backward_position = i;
            }
        }
    }

//    Check if the match is good
    if (customRatio(min_distance_backward, min_distance_forward) < TDC_MATCH_TRESHOLD) {
        char error_str[256];
        sprintf(error_str,
                "Failed to find a match. Forward: %" PRIu64 ". Backward: %" PRIu64 ".",
                min_distance_forward, min_distance_backward);
        logErrorAndExit(error_str);
    }

    if (min_distance_forward < min_distance_backward) {
        this->matching_clock = min_distance_forward_position + 1;
        this->box_to_match = 2;
    } else {
        this->matching_clock = min_distance_backward_position + 1;
        this->box_to_match = 1;
    }

    free(first_clock_deltas);
    free(second_clock_deltas);
}

void TDC_merger::merge(uint64_t max_fit_points) {
    uint64_t starting_index_first, starting_index_second;
    uint64_t matching_clock_first, matching_clock_second;

    if (this->box_to_match == 1) {
        starting_index_first = this->first_data->findNthClock(this->matching_clock);
        starting_index_second = this->second_data->findNthClock(1);
        matching_clock_first = this->matching_clock;
        matching_clock_second = 1;
    } else {
        starting_index_first = this->first_data->findNthClock(1);
        starting_index_second = this->second_data->findNthClock(this->matching_clock);
        matching_clock_first = 1;
        matching_clock_second = this->matching_clock;
    }

//    Shift the clock arrays so that the matching event on both is at time zero.
//    The events before the matching one are not going to be considered.
    uint64_t number_matching_clock_first = this->num_first_clocks - matching_clock_first;
    uint64_t number_matching_clock_second = this->num_second_clocks - matching_clock_second;

    uint64_t starting_clock_first = this->first_clocks[matching_clock_first];
    for (uint64_t i = 0; i < number_matching_clock_first; ++i) {
        this->first_clocks[i + matching_clock_first] -= starting_clock_first;
    }

    uint64_t starting_clock_second = this->second_clocks[matching_clock_second];
    for (uint64_t i = 0; i < number_matching_clock_second; ++i) {
        this->second_clocks[i + matching_clock_second] -= starting_clock_second;
    }

//    Find the common number of matched clock events.
    uint64_t common_number_clocks;
    if (number_matching_clock_first > number_matching_clock_second) {
        common_number_clocks = number_matching_clock_second;
    } else {
        common_number_clocks = number_matching_clock_first;
    }

//    If there are more common clock events than `max_fit_points`, use the latter to fit. Otherwise use the first.
    if (common_number_clocks > max_fit_points) common_number_clocks = max_fit_points;

    double x_times_y = 0., x_squared = 0.;
    for (uint64_t i = 1; i < common_number_clocks; ++i) {
        x_times_y += (double) this->first_clocks[matching_clock_first + i] *
                     this->second_clocks[matching_clock_second + i];
        x_squared += (double) this->first_clocks[matching_clock_first + i] *
                     this->first_clocks[matching_clock_first + i];
    }

    double correction_slope = x_times_y / x_squared;

    uint64_t *matched_first_timestamps = (uint64_t *) malloc(
            (this->first_data->getSize() - starting_index_first) * sizeof(uint64_t));
    uint64_t *matched_second_timestamps = (uint64_t *) malloc(
            (this->second_data->getSize() - starting_index_second) * sizeof(uint64_t));

    /*for (uint64_t i = starting_index_first; i < this->first_data->getSize(); ++i) {
        matched_first_timestamps[i] =
                this->first_data->getTimestamp(i) - this->first_data->getTimestamp(starting_index_first);
    }*/

    for (uint64_t i = 0; i < this->first_data->getSize() - starting_index_first; ++i) {
        matched_first_timestamps[i] =
                this->first_data->getTimestamp(i + starting_index_first) -
                        this->first_data->getTimestamp(starting_index_first);
    }

/*    for (uint64_t i = starting_index_second; i < this->second_data->getSize(); ++i) {
        matched_second_timestamps[i] =
                this->second_data->getTimestamp(i) - this->second_data->getTimestamp(starting_index_second);
        matched_second_timestamps[i] = (uint64_t) llround((double) matched_second_timestamps[i] / correction_slope);
    }*/

    for (uint64_t i = 0; i < this->second_data->getSize() - starting_index_second; ++i) {
        matched_second_timestamps[i] =
                this->second_data->getTimestamp(i + starting_index_second) -
                        this->second_data->getTimestamp(starting_index_second);
        matched_second_timestamps[i] = (uint64_t) llround((double) matched_second_timestamps[i] / correction_slope);
    }

    this->size = this->first_data->getSize() + this->second_data->getSize()
                 - starting_index_first - starting_index_second - number_matching_clock_second;
    this->timestamp = (uint64_t *) malloc(this->size * sizeof(uint64_t));
    this->channel = (uint16_t *) malloc(this->size * sizeof(uint16_t));

    uint64_t joint_index = 0, first_index = 0, second_index = 0;
    bool end_not_reached = true;
    do {
        if (first_index < this->first_data->getSize() - starting_index_first) {
            if (second_index < this->second_data->getSize() - starting_index_second) {
                while (this->second_data->isClock(second_index + starting_index_second)) {
                    second_index++;
                }

                if (matched_first_timestamps[first_index] < matched_second_timestamps[second_index]) {
                    this->timestamp[joint_index] = matched_first_timestamps[first_index];
                    this->channel[joint_index] = this->first_data->getChannel(first_index + starting_index_first) - 1;
                    first_index++;
                } else {
                    this->timestamp[joint_index] = matched_second_timestamps[second_index];
                    this->channel[joint_index] = this->second_data->getChannel(second_index + starting_index_second) - 1;
                    second_index++;
                }

                joint_index++;
            } else {
                this->timestamp[joint_index] = matched_first_timestamps[first_index];
                this->channel[joint_index] = this->first_data->getChannel(first_index + starting_index_first) - 1;
                first_index++;
            }
        } else {
            if (second_index < this->second_data->getSize() - starting_index_second) {
                while (this->second_data->isClock(second_index + starting_index_second)) {
                    second_index++;
                }

                this->timestamp[joint_index] = matched_second_timestamps[second_index];
                this->channel[joint_index] = this->second_data->getChannel(second_index + starting_index_second) - 1;
                second_index++;
                joint_index++;
            } else {
                end_not_reached = false;
            }
        }
    } while ((joint_index < this->size) && (this->timestamp[joint_index - 1] < TDC_ONE_SEC_BINS) && end_not_reached);

    this->size = joint_index;

    free(matched_first_timestamps);
    free(matched_second_timestamps);
}

uint16_t TDC_merger::getChannel(uint64_t index) const {
    return (uint16_t) (this->channel[index] + 1);
}


