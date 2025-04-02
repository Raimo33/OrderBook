/*================================================================================

File: OrderBook.hpp                                                             
Creator: Claudio Raimondi                                                       
Email: claudio.raimondi@pm.me                                                   

created at: 2025-03-22 14:14:57                                                 
last edited: 2025-04-02 21:57:52                                                

================================================================================*/

#pragma once

#include <cstdint>
#include <array>
#include <vector>
#include <ska/bytell_hash_map.hpp>

#include "macros.hpp"

class OrderBook
{
  public:
    OrderBook(void) noexcept;
    OrderBook(OrderBook &&other) noexcept;
    ~OrderBook(void);

    enum Side : uint8_t { BID = 0, ASK = 1 };

    typedef struct 
    {
      int32_t price;
      uint64_t qty;
    } BookEntry;

    inline uint64_t getId(void) const noexcept;

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

    template <typename Comparator> 
    std::vector<int32_t>::const_iterator findPrice(const std::vector<int32_t> &prices, const int32_t price) const;

    void removeOrder(std::vector<int32_t> &prices, std::vector<uint64_t> &qtys, const BookEntry &order);

    const uint32_t id;

    ska::bytell_hash_map<uint64_t, BookEntry> orders;
    std::array<std::vector<int32_t>, 2> price_arrays;
    std::array<std::vector<uint64_t>, 2> qty_arrays;
    int32_t equilibrium_price;
    uint64_t equilibrium_bid_qty;
    uint64_t equilibrium_ask_qty;
};

#include "OrderBook.inl"