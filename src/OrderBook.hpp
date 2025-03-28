/*================================================================================

File: OrderBook.hpp                                                             
Creator: Claudio Raimondi                                                       
Email: claudio.raimondi@pm.me                                                   

created at: 2025-03-22 14:14:57                                                 
last edited: 2025-03-28 22:11:56                                                

================================================================================*/

#pragma once

#include <cstdint>
#include <array>
#include <vector>
#include <unordered_map>

#include "macros.hpp"

class OrderBook
{
  public:
    OrderBook(void);
    ~OrderBook(void);

    enum Side { BID = 0, ASK = 1 };

    inline std::pair<uint32_t, uint32_t> getBestPrices(void) const;

    void addOrder(const uint64_t id, const Side side, const uint32_t price, const uint64_t volume);
    void removeOrder(const uint64_t id, const Side side);
    void executeOrder(const uint64_t id, const Side side, const uint64_t volume);

  private:

    template <typename Compare>
    std::vector<uint32_t>::const_iterator findPrice(const std::vector<uint32_t> &prices, const uint32_t price, Compare comp) const;

    typedef struct 
    {
      uint32_t price;
      uint64_t volume;
    } Order;

    std::unordered_map<uint64_t, Order> orders; //TODO optimize speed of hashmap. default uses buckets...

    std::vector<uint32_t> price_arrays[2];
    std::vector<uint64_t> volume_arrays[2];
};

#include "OrderBook.inl"