#pragma once

#include <cstdint>
#include <vector>

#include "macros.hpp"

class OrderBook
{
  public:
    OrderBook(void);
    ~OrderBook(void);

    enum Side { BID = 0, ASK = 1 };

    std::pair<uint32_t, uint32_t> getBestPrices(void) const;

    void addOrder(const Side side, const uint32_t price, const uint64_t volume);
    void removeOrder(const Side side, const uint32_t price, const uint64_t volume);
    void executeOrder(const Side side, UNUSED const uint32_t price, const uint64_t volume);

  private:

    template <typename Compare>
    std::vector<uint32_t>::const_iterator findPrice(const std::vector<uint32_t> &prices, const uint32_t price, Compare comp) const;

    std::vector<uint32_t> price_arrays[2];
    std::vector<uint64_t> volume_arrays[2];

};