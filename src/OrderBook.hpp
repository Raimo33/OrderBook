/*================================================================================

File: OrderBook.hpp                                                             
Creator: Claudio Raimondi                                                       
Email: claudio.raimondi@pm.me                                                   

created at: 2025-03-22 14:14:57                                                 
last edited: 2025-03-23 17:58:46                                                

================================================================================*/

#pragma once

#include <cstdint>
#include <array>
#include <vector>

#include "macros.hpp"

class OrderBook
{
  public:
    OrderBook(void);
    ~OrderBook(void);

    enum Side { BID = 0, ASK = 1 };

    inline std::pair<uint32_t, uint32_t> getBestPrices(void) const;

    void addOrder(const Side side, const uint32_t price, const uint64_t volume);
    void removeOrder(const Side side, const uint32_t price, const uint64_t volume);
    void executeOrder(const Side side, UNUSED const uint32_t price, const uint64_t volume);

  private:

    template <typename Compare>
    std::vector<uint32_t>::const_iterator findPrice(const std::vector<uint32_t> &prices, const uint32_t price, Compare comp) const;

    std::array<std::vector<uint32_t>, 2> price_arrays;
    std::array<std::vector<uint64_t>, 2> volume_arrays;
};

#include "OrderBook.inl"