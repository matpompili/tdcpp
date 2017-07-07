/*
 * Created by Matteo Pompili on 4/24/17.
 * MSc. Physics Student @ La Sapienza
 * */

#include <iostream>
#include "TDCpp_merger.h"

TDCpp_merger::TDCpp_merger(TDCpp_data *first_data, TDCpp_data *second_data) {
    this->first_data = first_data;
    this->second_data = second_data;

    /// This is needed to ensure channels are named correctly.
    this->box_number = 1;

    this->clock = this->first_data->get_clock_channel();
    this->num_channels = this->first_data->get_channels_number() + this->second_data->get_channels_number();

    /// Set the offsets to zero, if needed they are going to be loaded later.
    this->offset = (int16_t *) calloc(this->num_channels, sizeof(int16_t));

    /// Find the matching index between the two objects. Also find which object is delayed.
    this->find_match(200, 20);

    /// Join the two objects into one.
    this->merge(100);
}

TDCpp_merger::~TDCpp_merger() {
    this->first_data = nullptr;
    this->second_data = nullptr;

    free(this->first_clocks);
    free(this->second_clocks);
    free(this->offset);
}

void TDCpp_merger::find_match(uint64_t max_shift, uint64_t time_depth) {
    /// Allocate the arrays that will contains the clock events of the two objects.
    this->first_clocks = (uint64_t *) malloc(this->first_data->get_size() * sizeof(uint64_t));
    this->second_clocks = (uint64_t *) malloc(this->second_data->get_size() * sizeof(uint64_t));

    /// Get the clock events, as well as their count.
    this->num_first_clocks = this->first_data->get_clock_array(this->first_clocks);
    this->num_second_clocks = this->second_data->get_clock_array(this->second_clocks);

    /// Allocate the arrays for the time differences between clock events.
    uint64_t *first_clock_deltas = (uint64_t *) malloc((this->num_first_clocks - 1) * sizeof(uint64_t));
    uint64_t *second_clock_deltas = (uint64_t *) malloc((this->num_second_clocks - 1) * sizeof(uint64_t));

    /// Populate the delta arrays
    for (uint64_t i = 0; i < this->num_first_clocks - 1; ++i) {
        first_clock_deltas[i] = this->first_clocks[i + 1] - this->first_clocks[i];
    }
    for (uint64_t i = 0; i < this->num_second_clocks - 1; ++i) {
        second_clock_deltas[i] = this->second_clocks[i + 1] - this->second_clocks[i];
    }

    /// Find what is the maximum possible shift between the objects, given the time depth.
    if (this->num_first_clocks - time_depth < max_shift) max_shift = this->num_first_clocks - time_depth;
    if (this->num_second_clocks - time_depth < max_shift) max_shift = this->num_second_clocks - time_depth;

    uint64_t distance_forward, distance_backward;
    /// The following variables will be properly initialized in the following loop, this
    /// is only needed for the warnings the compiler raises.
    uint64_t min_distance_forward = 0, min_distance_forward_position = 0;
    uint64_t min_distance_backward = 0, min_distance_backward_position = 0;
    bool min_distance_changed;

    /// Evaluate the time differences.
    for (uint64_t i = 0; i < max_shift; ++i) {
        min_distance_changed = false;
        distance_backward = distance_forward = 0;
        for (uint64_t j = 0; j < time_depth; ++j) {
            /// Here #i is the delay between the objects. #j is the time index.
            distance_forward += abs_diff_64(first_clock_deltas[j], second_clock_deltas[i + j]);
            distance_backward += abs_diff_64(first_clock_deltas[i + j], second_clock_deltas[j]);
        }

        /// Need just on the first step.
        if (i == 0) {
            min_distance_forward = distance_forward;
            min_distance_forward_position = 0;
            min_distance_backward = distance_backward;
            min_distance_backward_position = 0;
            min_distance_changed = true;
        } else {
            if (distance_forward < min_distance_forward) {
                min_distance_forward = distance_forward;
                min_distance_forward_position = i;
                min_distance_changed = true;
            }

            if (distance_backward < min_distance_backward) {
                min_distance_backward = distance_backward;
                min_distance_backward_position = i;
                min_distance_changed = true;
            }
        }

        if (min_distance_changed) {
            if (custom_ratio(min_distance_backward, min_distance_forward) >= TDCPP_MATCH_THRESHOLD) i = max_shift;
        }


    }

    /// Check if the match is good
    if (custom_ratio(min_distance_backward, min_distance_forward) < TDCPP_MATCH_THRESHOLD) {
        char error_str[256];
        sprintf(error_str,
                "Failed to find a match. Forward: %" PRIu64 ". Backward: %" PRIu64 ".",
                min_distance_forward, min_distance_backward);
        log_error_and_exit(error_str);
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

void TDCpp_merger::merge(uint64_t max_fit_points) {
    uint64_t starting_index_first, starting_index_second;
    uint64_t matching_clock_first, matching_clock_second;

    /// Find the index of the first common clock in the two objects (actually is the second, since find_match
    /// returns the second one)
    if (this->box_to_match == 1) {
        starting_index_first = this->first_data->find_nth_clock(this->matching_clock);
        matching_clock_first = this->matching_clock;
        starting_index_second = this->second_data->find_nth_clock(1);
        matching_clock_second = 1;
    } else {
        starting_index_first = this->first_data->find_nth_clock(1);
        matching_clock_first = 1;
        starting_index_second = this->second_data->find_nth_clock(this->matching_clock);
        matching_clock_second = this->matching_clock;
    }

    /// Shift the clock arrays so that the matching event on both is at time zero.
    /// The events before the matching one are not going to be considered or shifted.
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

    /// Find the common number of matched clock events, i.e. the minimum of the two.
    uint64_t common_number_clocks;
    if (number_matching_clock_first > number_matching_clock_second) {
        common_number_clocks = number_matching_clock_second;
    } else {
        common_number_clocks = number_matching_clock_first;
    }

    /// If there are more common clock events than #max_fit_points, use the latter to fit. Otherwise use the first.
    if (common_number_clocks > max_fit_points) common_number_clocks = max_fit_points;

    /// Perform a linear regression, without intercept.
    /// We will be using the power series 1/(1-x) ~ 1 + x + O(x^2) to correct the derive, since x
    /// is expected to be O(1E-6) or less.
    double x_times_y = 0., x_squared = 0.;
    for (uint64_t i = 1; i < common_number_clocks; ++i) {
        x_times_y +=
                (double) this->first_clocks[matching_clock_first + i] * this->second_clocks[matching_clock_second + i];
        x_squared +=
                (double) this->first_clocks[matching_clock_first + i] * this->first_clocks[matching_clock_first + i];
    }
    double correction_factor =  1. - x_times_y / x_squared;

    /// Allocate space for the matched arrays
    uint64_t *matched_first_timestamps =
            (uint64_t *) malloc((this->first_data->get_size() - starting_index_first) * sizeof(uint64_t));
    uint64_t *matched_second_timestamps =
            (uint64_t *) malloc((this->second_data->get_size() - starting_index_second) * sizeof(uint64_t));


    /// Shift the timestamps of the first (before we shifted only the clock)
    for (uint64_t i = 0; i < this->first_data->get_size() - starting_index_first; ++i) {
        matched_first_timestamps[i] =
                this->first_data->get_timestamp(i + starting_index_first) -
                this->first_data->get_timestamp(starting_index_first);
    }

/*    this->first_data->copy_timestamp_array(matched_first_timestamps,
                                           starting_index_first, this->first_data->get_size() - starting_index_first);

    uint64_t foo = this->first_data->get_timestamp(starting_index_first);

    auto shift_func = [&] (uint64_t ts) {
        return ts - foo;
    };

    u64_vectorize_function(matched_first_timestamps, (this->first_data->get_size() - starting_index_first),  shift_func);*/


    /// Shift the timestamps of the second and correct for the time drift.
    for (uint64_t i = 0; i < this->second_data->get_size() - starting_index_second; ++i) {
        matched_second_timestamps[i] =
                this->second_data->get_timestamp(i + starting_index_second) -
                this->second_data->get_timestamp(starting_index_second);
        matched_second_timestamps[i] += (uint64_t) trunc((double) matched_second_timestamps[i] * correction_factor);
    }

/*    this->second_data->copy_timestamp_array(matched_second_timestamps,
                                            starting_index_second, this->second_data->get_size() - starting_index_second);

    foo = this->second_data->get_timestamp(starting_index_second);

    auto shift_and_drift_func = [&] (uint64_t ts){
        ts = ts - foo;
        return  ts + (uint64_t) trunc((double) ts * correction_factor);
    };

    u64_vectorize_function(matched_second_timestamps,
                           (this->second_data->get_size() - starting_index_second), shift_and_drift_func);*/

    this->size = this->first_data->get_size() + this->second_data->get_size()
                 - starting_index_first - starting_index_second - number_matching_clock_second;
    this->timestamp = (uint64_t *) malloc(this->size * sizeof(uint64_t));
    this->channel = (uint16_t *) malloc(this->size * sizeof(uint16_t));

    uint64_t joint_index = 0, first_index = 0, second_index = 0;
    bool end_reached = false;

    /// Merge and sort the two arrays at the same time. Much faster than merging and then sorting. This in O(n).
    do {
        /// While we have events in the first array
        if (first_index < this->first_data->get_size() - starting_index_first) {
            /// While we have events in the second array
            if (second_index < this->second_data->get_size() - starting_index_second) {
                /// If the event is a clock one, skip it. We are going to keep only the clock events of the first one.
                while (this->second_data->is_clock(second_index + starting_index_second)) {
                    second_index++;
                }

                /// Add the lesser of the two timestamps.
                if (matched_first_timestamps[first_index] < matched_second_timestamps[second_index]) {
                    this->timestamp[joint_index] = matched_first_timestamps[first_index];
                    /// We need to make this casting, otherwise the compiler complains.
                    this->channel[joint_index] = (uint16_t)
                            (this->first_data->get_channel(first_index + starting_index_first) - 1);
                    first_index++;
                } else {
                    this->timestamp[joint_index] = matched_second_timestamps[second_index];
                    this->channel[joint_index] = (uint16_t)
                            (this->second_data->get_channel(second_index + starting_index_second) - 1);
                    second_index++;
                }

                joint_index++;
            } else {
                /// Copy from the first array, the second is finished.
                this->timestamp[joint_index] = matched_first_timestamps[first_index];
                this->channel[joint_index] = (uint16_t)
                        (this->first_data->get_channel(first_index + starting_index_first) - 1);
                first_index++;
            }
        } else {
            /// Copy from the second array, if is not a clock one. The first array is finished.
            if (second_index < this->second_data->get_size() - starting_index_second) {
                while (this->second_data->is_clock(second_index + starting_index_second)) {
                    second_index++;
                }

                this->timestamp[joint_index] = matched_second_timestamps[second_index];
                this->channel[joint_index] = (uint16_t)
                        (this->second_data->get_channel(second_index + starting_index_second) - 1);
                second_index++;
                joint_index++;
            } else {
                end_reached = true;
            }
        }
    } while ((joint_index < this->size) && (this->timestamp[joint_index - 1] < TDCPP_ONE_SEC_BINS) && not end_reached);

    /// This is the actual size of the joint array.
    this->size = joint_index;

    free(matched_first_timestamps);
    free(matched_second_timestamps);
}
