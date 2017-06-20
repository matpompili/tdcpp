/*
 * Created by Matteo Pompili on 4/24/17.
 * MSc. Physics Student @ La Sapienza
 * */
#ifndef DOPPIE_CPP_TDC_MERGER_H
#define DOPPIE_CPP_TDC_MERGER_H


#include "TDC_data.h"
#include <cmath>

class TDC_merger : public TDC_data {

    TDC_data *first_data, *second_data;
    uint64_t *first_clocks, *second_clocks;
    uint64_t num_first_clocks, num_second_clocks;
    uint64_t matching_clock;
    uint8_t box_to_match;

public:
    TDC_merger(TDC_data *first_data, TDC_data *second_data);

    virtual ~TDC_merger();

    uint16_t getChannel(uint64_t index) const override;

private:
    void findMatch(uint64_t max_shift, uint64_t time_depth);

    void merge(uint64_t max_fit_points);

    uint64_t abs_diff_64(uint64_t x, uint64_t y);
};


#endif //DOPPIE_CPP_TDC_MERGER_H
