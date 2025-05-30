/*================================================================================

File: OrderBook.inl                                                             
Creator: Claudio Raimondi                                                       
Email: claudio.raimondi@pm.me                                                   

created at: 2025-03-23 17:58:46                                                 
last edited: 2025-04-30 15:40:07                                                

================================================================================*/

#pragma once

#include "OrderBook.hpp"
#include "macros.hpp"
#include "error.hpp"

extern volatile bool error;

HOT ALWAYS_INLINE inline int32_t OrderBook::getBestBidPrice(void) const noexcept
{
  return book_sides[BID].prices.back();
}

HOT ALWAYS_INLINE inline int32_t OrderBook::getBestAskPrice(void) const noexcept
{
  return book_sides[ASK].prices.back();
}

HOT ALWAYS_INLINE inline uint64_t OrderBook::getBestBidQty(void) const noexcept
{
  return book_sides[BID].cumulative_qtys.back();
}

HOT ALWAYS_INLINE inline uint64_t OrderBook::getBestAskQty(void) const noexcept
{
  return book_sides[ASK].cumulative_qtys.back();
}

HOT ALWAYS_INLINE inline int32_t OrderBook::getEquilibriumPrice(void) const noexcept
{
  return equilibrium_price;
}

HOT ALWAYS_INLINE inline uint64_t OrderBook::getEquilibriumBidQty(void) const noexcept
{
  return equilibrium_bid_qty;
}

HOT ALWAYS_INLINE inline uint64_t OrderBook::getEquilibriumAskQty(void) const noexcept
{
  return equilibrium_ask_qty;
}

inline void OrderBook::setEquilibrium(const int32_t price, const uint64_t bid_qty, const uint64_t ask_qty) noexcept
{
  equilibrium_price = price;
  equilibrium_bid_qty = bid_qty;
  equilibrium_ask_qty = ask_qty;
}

HOT ALWAYS_INLINE inline void OrderBook::addOrderBid(const uint64_t id, const int32_t price, const uint64_t qty)
{
  addOrder<std::less_equal<int32_t>>(book_sides[BID], id, price, qty);
}

HOT ALWAYS_INLINE inline void OrderBook::addOrderAsk(const uint64_t id, const int32_t price, const uint64_t qty)
{
  addOrder<std::greater_equal<int32_t>>(book_sides[ASK], id, price, qty);
}

HOT ALWAYS_INLINE inline void OrderBook::removeOrderBid(const uint64_t id, const int32_t price, const uint64_t qty)
{
  removeOrder<std::less_equal<int32_t>>(book_sides[BID], id, price, qty);
}

HOT ALWAYS_INLINE inline void OrderBook::removeOrderAsk(const uint64_t id, const int32_t price, const uint64_t qty)
{
  removeOrder<std::greater_equal<int32_t>>(book_sides[ASK], id, price, qty);
}
