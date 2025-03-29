/*================================================================================

File: OrderBook.cpp                                                             
Creator: Claudio Raimondi                                                       
Email: claudio.raimondi@pm.me                                                   

created at: 2025-03-07 21:17:51                                                 
last edited: 2025-03-29 23:01:33                                                

================================================================================*/

#include "OrderBook.hpp"
#include "macros.hpp"
#include "error.hpp"

extern volatile bool error;

COLD OrderBook::OrderBook(void)
{
  price_arrays[BID].push_back(0);
  price_arrays[ASK].push_back(UINT32_MAX);
  qty_arrays[BID].push_back(0);
  qty_arrays[ASK].push_back(UINT64_MAX);
}

COLD OrderBook::~OrderBook(void) {}

HOT void OrderBook::addOrder(const uint64_t id, const Side side, const uint32_t price, const uint64_t qty)
{
  std::vector<uint32_t> &prices = price_arrays[side];
  std::vector<uint64_t> &qtys = qty_arrays[side];

  orders.emplace(id, Order{price, qty});

  constexpr bool (*comparators[])(const uint32_t, const uint32_t) = {
    [](const uint32_t a, const uint32_t b) { return a < b; },
    [](const uint32_t a, const uint32_t b) { return a > b; }
  };

  const auto price_it = findPrice(prices, price, comparators[side]);
  const auto qty_it = qtys.begin() + std::distance(prices.cbegin(), price_it);

  if (LIKELY(price_it != prices.end()) && (*price_it == price))
    *qty_it += qty;
  else
  {
    prices.insert(price_it, price);
    qtys.insert(qty_it, qty);
  }
}

HOT void OrderBook::removeOrder(const uint64_t id, const Side side)
{
  std::vector<uint32_t> &prices = price_arrays[side];
  std::vector<uint64_t> &qtys = qty_arrays[side];

  const auto it = orders.find(id);
  const Order &order = it->second;
  orders.erase(it);

  const auto price_it = findPrice(prices, order.price, std::not_equal_to<uint32_t>());
  const auto qty_it = qtys.begin() + std::distance(prices.cbegin(), price_it);

  auto &available_qty = *qty_it;
  available_qty -= order.qty;

  if (UNLIKELY(available_qty) == 0)
  {
    prices.erase(price_it);
    qtys.erase(qty_it);
  }
}

HOT void OrderBook::executeOrder(const uint64_t id, const Side side, uint64_t qty)
{
  std::vector<uint32_t> &prices = price_arrays[side];
  std::vector<uint64_t> &qtys = qty_arrays[side];

  orders.erase(id);

  auto &available_qty = qtys.back();
  available_qty -= qty;

  if (UNLIKELY(available_qty == 0))
  {
    prices.pop_back();
    qtys.pop_back();
  }
}

template <typename Compare>
HOT std::vector<uint32_t>::const_iterator OrderBook::findPrice(const std::vector<uint32_t> &prices, const uint32_t price, Compare comp) const
{
  auto it = prices.crbegin();
  auto end = prices.crend();

  //TOOD SIMD backwards, 16 elements at once
  while (it != end && comp(*it, price))
    ++it;

  return it.base();
}