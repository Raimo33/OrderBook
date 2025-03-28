/*================================================================================

File: OrderBook.cpp                                                             
Creator: Claudio Raimondi                                                       
Email: claudio.raimondi@pm.me                                                   

created at: 2025-03-07 21:17:51                                                 
last edited: 2025-03-28 22:11:56                                                

================================================================================*/

#include "OrderBook.hpp"
#include "macros.hpp"
#include "error.hpp"

extern volatile bool error;

COLD OrderBook::OrderBook(void)
{
  for (auto &prices : price_arrays)
    prices.reserve(1024);
  for (auto &volumes : volume_arrays)
    volumes.reserve(1024);

  price_arrays[BID].push_back(0);
  price_arrays[ASK].push_back(UINT32_MAX);
  volume_arrays[BID].push_back(0);
  volume_arrays[ASK].push_back(UINT64_MAX);
}

COLD OrderBook::~OrderBook(void) {}

HOT void OrderBook::addOrder(const uint64_t id, const Side side, const uint32_t price, const uint64_t volume)
{
  std::vector<uint32_t> &prices = price_arrays[side];
  std::vector<uint64_t> &volumes = volume_arrays[side];

  orders[id] = { price, volume };

  constexpr bool (*comparators[])(const uint32_t, const uint32_t) = {
    [](const uint32_t a, const uint32_t b) { return a < b; },
    [](const uint32_t a, const uint32_t b) { return a > b; }
  };

  auto price_it = findPrice(prices, price, comparators[side]);
  auto volume_it = volumes.begin() + std::distance(prices.cbegin(), price_it);

  if (LIKELY(price_it != prices.end() && *price_it == price))
    *volume_it += volume;
  else
  {
    prices.insert(price_it, price);
    volumes.insert(volume_it, volume);
  }
}

HOT void OrderBook::removeOrder(const uint64_t id, const Side side)
{
  std::vector<uint32_t> &prices = price_arrays[side];
  std::vector<uint64_t> &volumes = volume_arrays[side];

  const auto it = orders.find(id);
  const Order &order = it->second;
  orders.erase(it);

  auto price_it = findPrice(prices, order.price, std::not_equal_to<uint32_t>());
  auto volume_it = volumes.begin() + std::distance(prices.cbegin(), price_it);

  auto &available_volume = *volume_it;
  available_volume -= order.volume;

  if (UNLIKELY(available_volume == 0))
  {
    prices.erase(price_it);
    volumes.erase(volume_it);
  }
}

HOT void OrderBook::executeOrder(const uint64_t id, const Side side, uint64_t volume)
{
  std::vector<uint32_t> &prices = price_arrays[side];
  std::vector<uint64_t> &volumes = volume_arrays[side];

  orders.erase(id);

  auto &available_volume = volumes.back();
  available_volume -= volume;

  if (UNLIKELY(available_volume == 0))
  {
    prices.pop_back();
    volumes.pop_back();
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