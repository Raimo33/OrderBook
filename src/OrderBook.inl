/*================================================================================

File: OrderBook.inl                                                             
Creator: Claudio Raimondi                                                       
Email: claudio.raimondi@pm.me                                                   

created at: 2025-03-23 17:58:46                                                 
last edited: 2025-03-23 17:58:46                                                

================================================================================*/

#pragma once

#include "OrderBook.hpp"
#include "macros.hpp"
#include "error.hpp"

extern volatile bool error;

HOT inline std::pair<uint32_t, uint32_t> OrderBook::getBestPrices(void) const
{
  return std::make_pair(price_arrays[BID].back(), price_arrays[ASK].back());
}