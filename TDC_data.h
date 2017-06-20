/*
 * Created by Matteo Pompili on 4/22/17.
 * MSc. Physics Student @ La Sapienza
 * */

#ifndef TDC_DATA_H
#define TDC_DATA_H

/*
 * This class is used to read timestamps data from ID800-TDC
 * It also implements the method used to find the n-fold coincidences.
 * */

#include <stdint-gcc.h>
#include <cstdio>
#include <cstdlib>
#include <map>
#include <cinttypes>
#include "TDC_utils.h"

/*
 * The timestamps file has a 40 byte header that has to be skipped
 * */
#define TDC_HEADER_SIZE 40

/*
 * An event is made of:
 *  - timestamp:  8byte,
 *  - channel:    2byte,
 * Total: 10byte.
 * */
#define TDC_TIMESTAMP_SIZE 8
#define TDC_CHANNEL_SIZE 2
#define TDC_RECORD_SIZE (TDC_TIMESTAMP_SIZE + TDC_CHANNEL_SIZE)

/*
 * A bin is equivalent to 81ps (on average).
 * */
#define TDC_BIN_SIZE 81E-12

/*
 * In one second (1s) there are 12345679012 bins.
 * */
#define TDC_ONE_SEC_BINS 12345679012


class TDC_data {

/*
 * The following members are protected so they can be accessed directly from
 * derived class.
 *
 *  - *timestamp:   u64 pointer
 *      the pointed array contains the timestamps.
 *      
 *  - *channel:     u16 pointer
 *      the pointed array contains the channels.
 *      
 *  - size:         u64
 *      the number of events in the object. Also the size of the arrays pointed by *timestamp and *channel.
 *      
 *  - max_channel:  u16
 *      the highest channel number.
 *      
 *  - clock:      u16
 *      the channel that is used as clock.
 *      
 *  - box_number:   u16
 *      the box from which the data comes from. This is used to give the correct channel number,
 *      check the getChannel method for further information.
 *      
 *  - data_file:    FILE pointer
 *      the binary file from which the data is read.
 * */

protected:
    uint64_t *timestamp;
    uint16_t *channel;
    uint64_t size;
    uint16_t max_channel;
    uint16_t clock;
    uint16_t box_number;
    FILE    *data_file;

/*
 * The following methods are public, this means that they can be called anywhere.
 * Documentation about them is available in TDC_data.cpp
 * */

public:
    TDC_data();

    virtual ~TDC_data();

    void loadFromFile(const char *data_file_path,
                      uint16_t clock,
                      uint16_t box_number);

    uint64_t getTimestamp(uint64_t index) const;

    virtual uint16_t getChannel(uint64_t index) const;

    uint16_t getBoxNumber() const;

    uint64_t getSize() const;

    uint16_t getClockChannel() const;

    uint64_t getClocks(uint64_t *destination_array);

    uint64_t findNthClock(uint64_t clock);

    bool isClock(uint64_t index) const;

    uint16_t getMaxChannel() const;

    void findNfoldCoincidences(uint16_t n,
                               const char *singles_file_name,
                               const char *coincidences_file_name,
                               uint64_t coincidence_window);

/*
 * The following methods are public, this means that they can be called anywhere.
 * Documentation about them is available in TDC_data.cpp
 * */

private:
    uint64_t getFileSize();

    uint64_t findOneSecondIndex();
};

#endif //TDC_DATA_H