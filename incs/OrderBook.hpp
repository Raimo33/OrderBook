/*================================================================================

File: OrderBook.hpp                                                             
Creator: Claudio Raimondi                                                       
Email: claudio.raimondi@pm.me                                                   

created at: 2025-03-22 14:14:57                                                 
last edited: 2025-04-10 20:10:41                                                

================================================================================*/

#pragma once

#include <cstdint>
#include <array>
#include <vector>

#include "macros.hpp"

class OrderBook
{
  public:
    OrderBook(void) noexcept;
    OrderBook(OrderBook &&) noexcept;
    ~OrderBook();

    enum Side : uint8_t { BID = 0, ASK = 1 };

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

    struct PriceLevels
    {
      //sorted with best price last. prices[i] and cumulative_qtys[i] are the same price level
      std::vector<int32_t> prices;
      std::vector<uint64_t> cumulative_qtys;

      //unsorted, order_ids[i][j] and order_qtys[i][j] are the same order
      std::vector<std::vector<uint64_t>> order_ids;
      std::vector<std::vector<uint64_t>> order_qtys;
    };

    PriceLevels bids;
    PriceLevels asks;

    int32_t equilibrium_price;
    uint64_t equilibrium_bid_qty;
    uint64_t equilibrium_ask_qty;

    void addOrderBid(const uint64_t id, const int32_t price, const uint64_t qty);
    void addOrderAsk(const uint64_t id, const int32_t price, const uint64_t qty);
    template <typename Compare>
    void addOrder(PriceLevels &levels, const uint64_t id, const int32_t price, const uint64_t qty);

    void removeOrderBid(const uint64_t id);
    void removeOrderAsk(const uint64_t id);
    void removeOrder(PriceLevels &levels, const uint64_t id);

    void removeOrderBid(const uint64_t id, const int32_t price, const uint64_t qty);
    void removeOrderAsk(const uint64_t id, const int32_t price, const uint64_t qty);
    template <typename Compare>
    void removeOrder(PriceLevels &levels, const uint64_t id, const int32_t price, const uint64_t qty);

    void executeOrderBid(const uint64_t id, const uint64_t qty);
    void executeOrderAsk(const uint64_t id, const uint64_t qty);
    void executeOrder(PriceLevels &levels, const uint64_t id, const uint64_t qty);

    void removeOrderFromPriceLevel(PriceLevels &levels, const size_t price_index, const uint64_t id);
    void removePriceLevel(PriceLevels &levels, const size_t price_index);
};

#include "OrderBook.inl"