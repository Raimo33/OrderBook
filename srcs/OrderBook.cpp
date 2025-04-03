/*================================================================================

File: OrderBook.cpp                                                             
Creator: Claudio Raimondi                                                       
Email: claudio.raimondi@pm.me                                                   

created at: 2025-03-07 21:17:51                                                 
last edited: 2025-04-03 20:16:29                                                

================================================================================*/

#include "OrderBook.hpp"
#include "utils/utils.hpp"
#include "macros.hpp"
#include "error.hpp"

extern volatile bool error;

COLD OrderBook::OrderBook(void) noexcept :
  equilibrium_price(INT32_MIN),
  equilibrium_bid_qty(INT32_MIN)
{
  bids.prices.push_back(INT32_MIN);
  asks.prices.push_back(INT32_MAX);
  bids.cumulative_qtys.push_back(0);
  asks.cumulative_qtys.push_back(0);
}

COLD OrderBook::OrderBook(OrderBook &&other) noexcept :
  equilibrium_price(other.equilibrium_price),
  equilibrium_bid_qty(other.equilibrium_bid_qty),
  bids(std::move(other.bids)),
  asks(std::move(other.asks))
{
}

HOT void OrderBook::addOrder(const uint64_t id, const Side side, const int32_t price, const uint64_t qty)
{
  //TODO
}

HOT void OrderBook::removeOrder(const uint64_t id, const Side side)
{
  //TODO
}

HOT void OrderBook::removeOrder(const uint64_t id, const Side side, const int32_t price, const uint64_t qty)
{
  //TODO
}

HOT void OrderBook::executeOrder(const uint64_t id, const Side side, uint64_t qty)
{
  //TODO
}