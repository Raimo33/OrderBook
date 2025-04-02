/*================================================================================

File: OrderBook.inl                                                             
Creator: Claudio Raimondi                                                       
Email: claudio.raimondi@pm.me                                                   

created at: 2025-03-23 17:58:46                                                 
last edited: 2025-04-02 21:57:52                                                

================================================================================*/

#pragma once

#include "OrderBook.hpp"
#include "macros.hpp"
#include "error.hpp"

extern volatile bool error;

COLD inline uint64_t OrderBook::getId(void) const noexcept
{
  return id;
}

HOT inline OrderBook::BookEntry OrderBook::getBestBid(void) const noexcept
{
  return {price_arrays[BID].back(), qty_arrays[BID].back()};
}

HOT inline OrderBook::BookEntry OrderBook::getBestAsk(void) const noexcept
{
  return {price_arrays[ASK].back(), qty_arrays[ASK].back()};
}

HOT inline OrderBook::BookEntry OrderBook::getEquilibriumPriceBid(void) const noexcept
{
  return {equilibrium_price, equilibrium_bid_qty};
}

inline OrderBook::BookEntry OrderBook::getEquilibriumPriceAsk(void) const noexcept
{
  return {equilibrium_price, equilibrium_ask_qty};
}

inline void OrderBook::setEquilibrium(const int32_t price, const uint64_t bid_qty, const uint64_t ask_qty) noexcept
{
  equilibrium_price = price;
  equilibrium_bid_qty = bid_qty;
  equilibrium_ask_qty = ask_qty;
}
