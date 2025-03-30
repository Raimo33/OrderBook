/*================================================================================

File: OrderBook.hpp                                                             
Creator: Claudio Raimondi                                                       
Email: claudio.raimondi@pm.me                                                   

created at: 2025-03-22 14:14:57                                                 
last edited: 2025-03-30 19:04:28                                                

================================================================================*/

#pragma once

#include <cstdint>
#include <array>
#include <vector>
#include <unordered_map>
#include <ska/flat_hash_map.hpp>

#include "macros.hpp"

class OrderBook
{
  public:
    OrderBook(void);
    ~OrderBook(void);

    enum Side { BID = 0, ASK = 1 };

    typedef struct 
    {
      int32_t price;
      uint64_t qty;
    } BookEntry;

    inline BookEntry getBestBid(void) const noexcept;
    inline BookEntry getBestAsk(void) const noexcept;
    inline BookEntry getEquilibriumPriceBid(void) const noexcept;
    inline BookEntry getEquilibriumPriceAsk(void) const noexcept;

    void addOrder(const uint64_t id, const Side side, const int32_t price, const uint64_t qty);
    void removeOrder(const uint64_t id, const Side side);
    void removeOrder(const uint64_t id, const Side side, const int32_t price, const uint64_t qty);
    void executeOrder(const uint64_t id, const Side side, const uint64_t qty);

    inline void setEquilibrium(const int32_t price, const uint64_t bid_qty, const uint64_t ask_qty) noexcept;

  private:

    void removeOrder(std::vector<int32_t> &prices, std::vector<uint64_t> &qtys, const BookEntry &order);

    template <typename Compare>
    std::vector<int32_t>::const_iterator findPrice(const std::vector<int32_t> &prices, const int32_t price, Compare comp) const;

    ska::flat_hash_map<uint64_t, BookEntry> orders;

    std::vector<int32_t> price_arrays[2];
    std::vector<uint64_t> qty_arrays[2];

    int32_t equilibrium_price;
    uint64_t equilibrium_bid_qty;
    uint64_t equilibrium_ask_qty;
};

#include "OrderBook.inl"