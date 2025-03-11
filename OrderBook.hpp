#pragma once

#include <cstdint>
#include <vector>

class OrderBook
{
  public:
    OrderBook(void);
    ~OrderBook(void);

    enum Side { BID = 0, ASK = 1 };

    std::pair<uint32_t, uint32_t> getBestPrices(void) const;

    void addOrder(const Side side, const uint32_t price, const uint64_t volume);
    void removeOrder(const Side side, const uint32_t price, const uint64_t volume);
    void executeOrder(const Side side, uint64_t volume);

  private:
    typedef std::pair<uint32_t, uint64_t> PriceLevel;

    bool (*comparators[2])(const PriceLevel &lhs, const uint32_t price);

    template <class Compare> void addOrder(std::vector<PriceLevel> &levels, const uint32_t price, const uint64_t volume, Compare comp);
    template <class Compare> void removeOrder(std::vector<PriceLevel> &levels, const uint32_t price, const uint64_t volume, Compare comp);

    std::vector<PriceLevel> level_arrays[2];
};