/*================================================================================

File: OrderBook.inl                                                             
Creator: Claudio Raimondi                                                       
Email: claudio.raimondi@pm.me                                                   

created at: 2025-03-23 17:58:46                                                 
last edited: 2025-03-29 18:37:54                                                

================================================================================*/

#pragma once

#include "OrderBook.hpp"
#include "macros.hpp"
#include "error.hpp"

extern volatile bool error;

HOT inline std::pair<uint32_t, uint64_t> OrderBook::getBestBid(void) const noexcept
{
  return {price_arrays[BID].back(), qty_arrays[BID].back()};
}

HOT inline std::pair<uint32_t, uint64_t> OrderBook::getBestAsk(void) const noexcept
{
  return {price_arrays[ASK].back(), qty_arrays[ASK].back()};
}

HOT inline std::pair<uint32_t, uint64_t> OrderBook::getEquilibriumPriceBid(void) const noexcept
{
  return {equilibrium_price, equilibrium_bid_qty};
}

HOT inline std::pair<uint32_t, uint64_t> OrderBook::getEquilibriumPriceAsk(void) const noexcept
{
  return {equilibrium_price, equilibrium_ask_qty};
}

HOT inline void OrderBook::setEquilibrium(const uint32_t price, const uint64_t bid_qty, const uint64_t ask_qty) noexcept
{
  equilibrium_price = price;
  equilibrium_bid_qty = bid_qty;
  equilibrium_ask_qty = ask_qty;
}
