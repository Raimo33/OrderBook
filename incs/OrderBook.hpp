/*================================================================================

File: OrderBook.hpp                                                             
Creator: Claudio Raimondi                                                       
Email: claudio.raimondi@pm.me                                                   

created at: 2025-03-22 14:14:57                                                 
last edited: 2025-04-30 15:40:07                                                

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

    inline int32_t getBestBidPrice(void) const noexcept;
    inline int32_t getBestAskPrice(void) const noexcept;
    inline uint64_t getBestBidQty(void) const noexcept;
    inline uint64_t getBestAskQty(void) const noexcept;
    inline int32_t getEquilibriumPrice(void) const noexcept;
    inline uint64_t getEquilibriumBidQty(void) const noexcept;
    inline uint64_t getEquilibriumAskQty(void) const noexcept;

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

    std::array<PriceLevels, 2> book_sides;

    int32_t equilibrium_price;
    uint64_t equilibrium_bid_qty;
    uint64_t equilibrium_ask_qty;

    inline void addOrderBid(const uint64_t id, const int32_t price, const uint64_t qty);
    inline void addOrderAsk(const uint64_t id, const int32_t price, const uint64_t qty);
    template<typename Comparator>
    void addOrder(PriceLevels &levels, const uint64_t id, const int32_t price, const uint64_t qty);
    void addOrderToExistingPriceLevel(PriceLevels &levels, const size_t price_idx, UNUSED const int32_t price, const uint64_t id, const uint64_t qty);
    void addOrderToNewPriceLevel(PriceLevels &levels, const size_t price_idx, const int32_t price, const uint64_t id, const uint64_t qty);

    inline void removeOrderBid(const uint64_t id, const int32_t price, const uint64_t qty);
    inline void removeOrderAsk(const uint64_t id, const int32_t price, const uint64_t qty);
    template<typename Comparator>
    void removeOrder(PriceLevels &levels, const uint64_t id, const int32_t price, const uint64_t qty);
    void removeOrderFromPriceLevel(PriceLevels &levels, const size_t price_idx, const uint64_t id);
    void removePriceLevel(PriceLevels &levels, const size_t price_idx, const uint64_t id);
};

#include "OrderBook.inl"