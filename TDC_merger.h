/**
 * \class TDC_merger
 *
 * \brief This class is used to merge two TDC_data objects into one.
 * It uses a clock channel to match the two objects, and corrects the time drift with a linear fit.
 *
 * \author $Author: Matteo Pompili$
 *
 * Contact: matpompili\@ gmail.com
 *
 * Created on: Apr 24 2017
 */
#ifndef TDC_MERGER_H
#define TDC_MERGER_H

#include "TDC_data.h"
#include <cmath>

/**
 * A threshold to accept or reject the matching of two TDC_data objects.
 * This is the ratio between the cumulative sums of time_deltas.
 */
#define TDC_MATCH_THRESHOLD 1000

/**
 * \brief This class is used to merge two TDC_data objects into one.
 * It uses a clock channel to match the two objects, and corrects the time derive with a linear fit.
 */
class TDC_merger : public TDC_data {
protected:
    /**
     * A pointer to the first of the two objects that are going to be merged.
     */
    TDC_data *first_data;

    /**
     * A pointer to the second of the two objects that are going to be merged.
     */
    TDC_data *second_data;

    /**
     * A pointer to the clock array of the first object.
     */
    uint64_t *first_clocks;

    /**
     * A pointer to the clock array of the second object.
     */
    uint64_t *second_clocks;

    /**
     * The number of clocks in the first object, i.e. the size of #first_clocks
     */
    uint64_t num_first_clocks;

    /**
     * The number of clocks in the second object, i.e. the size of #second_clocks
     */
    uint64_t num_second_clocks;

    /**
     * Either 1 or 2. If it is 1, then #matching_clock refers to the first box. Otherwise it refers to the second box.
     */
    uint8_t box_to_match;

    /**
     * The index of the first common clock. It refers to box number #box_to_match.
     */
    uint64_t matching_clock;

public:
    /**
     * This is the default constructor.
     * @param first_data The first of the two TDC_data objects that are going to be merged.
     * @param second_data The second of the two TDC_data objects that are going to be merged.
     */
    TDC_merger(TDC_data *first_data, TDC_data *second_data);

    /**
     * This is the default destructor.
     */
    virtual ~TDC_merger();

private:
    /**
     * Find the first common clock event in the two TDC_data objects.
     * @param max_shift Max offset to consider.
     * This depend on both the rate of the clock and the starting delay of the boxes.
     * @param time_depth Size of the timestamps subarrays to confront.
     */
    void find_match(uint64_t max_shift, uint64_t time_depth);

    /**
     * Join the two objects into one, shifting the timestamps and correcting for the time drift with a linear fit.
     * @param max_fit_points Maximum number of clock events to use for the fit.
     */
    void merge(uint64_t max_fit_points);
};


#endif //TDC_MERGER_H
