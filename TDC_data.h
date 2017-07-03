/**
 * \class TDC_data
 *
 * \brief This class is used to read and use timestamps data from ID800-TDC.
 *
 * \author $Author: Matteo Pompili$
 *
 * Contact: matpompili\@ gmail.com
 *
 * Created on: Apr 22 2017
 */

#ifndef TDC_DATA_H
#define TDC_DATA_H

#include <stdint-gcc.h>
#include <cstdio>
#include <cstdlib>
#include <map>
#include <cinttypes>
#include "TDC_utils.h"

/**
 * The timestamps file has a 40 byte header that has to be skipped
 * */
#define TDC_HEADER_SIZE 40

/**
 * An event is made of:
 *  - timestamp:  8byte,
 *  - channel:    2byte,
 * Total: 10byte.
 * */
#define TDC_TIMESTAMP_SIZE 8
#define TDC_CHANNEL_SIZE 2
#define TDC_RECORD_SIZE (TDC_TIMESTAMP_SIZE + TDC_CHANNEL_SIZE)

/**
 * A bin is equivalent to 81ps (on average).
 * */
#define TDC_BIN_SIZE 81E-12

/**
 * In one second (1s) there are 12345679012 bins.
 * */
#define TDC_ONE_SEC_BINS 12345679012

/**
 * This class is used to read and use timestamps data from ID800-TDC.
 */
class TDC_data {

protected:
    /**
     * A pointer to the timestamp array.
     * */
    uint64_t *timestamp;

    /**
     * A pointer to the channel array.
     * */
    uint16_t *channel;

    /**
     * A pointer to the offset array.
     * */
    int16_t *offset;

    /**
     * @brief The number of events in the object.
     *
     * It is also the size of the arrays pointed by #timestamp and #channel.
     * */
    uint64_t size;

    /**
     * The number of channels in the object.
     * */
    uint16_t num_channels;

    /**
     * The channel that is used as a clock.
     * */
    uint16_t clock;

    /**
     * @brief The number of the box from which the data comes from.
     *
     * This is used to give the correct channel number, check the getChannel() method for further information.
     * */
    uint16_t box_number;

public:
    /**
     * This is the default constructor.
     * It is called only by derived classes.
     * */
    TDC_data();

    /**
     * This is the default destructor.
     * It frees the arrays.
     * */
    virtual ~TDC_data();

    /**
     * This is an additional constructor.
     * It must be used when you need to read a timestamp binary file.
     *
     * @param data_file_path The path of the timestamp file to be loaded.
     * @param clock The channel that is going to be used as clock.
     * @param box_number The number of the box the data come from.
     */
    void load_from_file(const char *data_file_path,
                        uint16_t clock,
                        uint16_t box_number);

    /**
     * @param index The index of the event
     * @return The timestamp of the event
     */
    uint64_t get_timestamp(uint64_t index) const;

    /**
     * This method takes into account the box number, it works as follows:
     * The first box has box_number = 1. The channels, as given by the ID800, go from 0 to max_channel-1.
     * They will be returned by this method as 1 to max_channel.
     * If the box_number is higher than 1, 8 * (box_number-1) will be added to the result.
     * It is worth noting that inside the channel array the channels are always stored as going from 0 to max_channel-1.
     *
     * @param index The index of the event
     * @return The channel of the event
     */
    virtual uint16_t get_channel(uint64_t index) const;

    /**
     * @return The number of the box
     */
    uint16_t get_box_number() const {
        return box_number;
    }

    /**
     * @return The number of events in the object
     */
    uint64_t get_size() const {
        return size;
    };

    /**
     * @return The channel of the clock
     */
    uint16_t get_clock_channel() const {
        return clock;
    }

    /**
     * A method to retrieve the clock events and their number.
     * @param destination_array A pointer to an array that will be filled with the clock events.
     *      Must be already allocated.
     * @return The number of clock events.
     */
    uint64_t get_clock_array(uint64_t *destination_array);

    /**
     * @param n The number of clock event to search for.
     * @return The index of the nth clock event.
     */
    uint64_t find_nth_clock(uint64_t n);

    /**
     * Check if the event in position #index is a clock.
     * @param index The index of the event
     * @return True if the event is a clock, False otherwise.
     */
    bool is_clock(uint64_t index) const;

    /**
     * @return The number of channels in the object.
     */
    uint16_t get_channels_number() const {
        return num_channels;
    }

    /**
     * @brief This method finds n-fold coincidences in the object.
     * It also find the number of single events on each channel.
     * @param n The *exact* number of events that must occur at the same time (modulo coincidence_window).
     *      If more, or less, than n events occur in the coincidence_window, the coincidence will be ignored.
     * @param singles_file_name The name of the file in which the single events count will be saved.
     * @param coincidences_file_name The name of the file in which the coincidence events will be saved.
     * @param coincidence_window The maximum time distance *in bins* in which two
     *      or more events are considered coincident.
     */
    void find_n_fold_coincidences(uint16_t n,
                                  const char *singles_file_name,
                                  const char *coincidences_file_name,
                                  uint64_t coincidence_window);

    /**
     * Print to file the timestamps and relative channels in the object.
     * @param output_file_path The name of the output file.
     */
    void print_data_to_file(const char *output_file_path);

    /**
     * @brief Set an offset per channel and reorder data if necessary.
     * We are using the Insertion Sort algorithm, which is O(n^2) in the worse-case, but is
     * roughly O(n) if the array is nearly sorted (which is our case). A visual comparison of
     * sorting algorithm can be found at https://www.toptal.com/developers/sorting-algorithms
     * @param offset_file_path The name of the offset file.
     */
    void set_channel_offset(const char *offset_file_path);

    /**
     * Copy the timestamp array to #dest_array
     * @param dest_array Destination array. If this is not big enough segmentation fault will occur.
     * @param start_index The index from which the copy will start.
     * @param n_events The number of events to copy, starting from #start_index/
     */
    void copy_timestamp_array(uint64_t* dest_array, uint64_t start_index, uint64_t n_events);

private:
    /**
     * This method finds the number of events inside the file.
     * @param data_file A pointer to an open file.
     * @return The number of events in the file.
     */
    uint64_t get_file_size(FILE *data_file);

    /**
     * This method finds the index at which one second was passed since the first event.
     * It uses binary search.
     *
     * @return The index corresponding to one second of real time.
     */
    uint64_t find_one_second_index();
};

#endif //TDC_DATA_H