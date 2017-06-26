/*
 * Created by Matteo Pompili on 4/24/17.
 * MSc. Physics Student @ La Sapienza
 * */
#ifndef TDC_MERGER_H
#define TDC_MERGER_H

/*
 * This class is used to merge two TDC_data objects into one.
 * It uses a clock channel to match the two objects, and corrects the time derive with a linear fit.
 * */

#include "TDC_data.h"
#include <cmath>

#define TDC_MATCH_TRESHOLD 1000

class TDC_merger : public TDC_data {
/*
 * - *first_data, *second_data:             TDC_data pointer
 *      pointers to the objects that are going to be merged.
 *
 * - *first_clocks, *second_clocks:         u64 pointer
 *      the pointed arrays contain the clocks of the relative boxes.
 *
 * - num_first_clocks, num_second_clocks:   u64
 *      the number of clock events, i.e. the size of the arrays pointed by  `*first_clocks` and  `*second_clocks`.
 *
 * - box_to_match:                          u8
 *      either 1 or 2. If it is 1, then `matching_clock` refers to the first box. Otherwise it refers to the second box.
 *
 * - matching_clock:                        u64
 *      the index of the first common clock. It refers to box number `box_to_match`.
 * */

    TDC_data *first_data, *second_data;
    uint64_t *first_clocks, *second_clocks;
    uint64_t num_first_clocks, num_second_clocks;
    uint8_t box_to_match;
    uint64_t matching_clock;

/*
 * The following methods are public, this means that they can be called anywhere.
 * Documentation about them is available in TDC_merger.cpp
 * */

public:
    TDC_merger(TDC_data *first_data, TDC_data *second_data);

    virtual ~TDC_merger();

    uint16_t getChannel(uint64_t index) const override;
/*
 * The following methods are private, this means that they can be called only by the object itself.
 * Documentation about them is available in TDC_merger.cpp
 * */

private:
    void findMatch(uint64_t max_shift, uint64_t time_depth);

    void merge(uint64_t max_fit_points);

    uint64_t abs_diff_64(uint64_t x, uint64_t y);
};


#endif //TDC_MERGER_H
