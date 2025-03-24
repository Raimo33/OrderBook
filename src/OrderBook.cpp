/*================================================================================

File: OrderBook.cpp                                                             
Creator: Claudio Raimondi                                                       
Email: claudio.raimondi@pm.me                                                   

created at: 2025-03-07 21:17:51                                                 
last edited: 2025-03-24 22:03:35                                                

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

HOT void OrderBook::addOrder(const Side side, const uint32_t price, const uint64_t volume)
{
  std::vector<uint32_t> &prices = price_arrays[side];
  std::vector<uint64_t> &volumes = volume_arrays[side];

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

HOT void OrderBook::removeOrder(const Side side, const uint32_t price, const uint64_t volume)
{
  std::vector<uint32_t> &prices = price_arrays[side];
  std::vector<uint64_t> &volumes = volume_arrays[side];

  auto price_it = findPrice(prices, price, std::not_equal_to<uint32_t>());
  auto volume_it = volumes.begin() + std::distance(prices.cbegin(), price_it);

  auto& value = *volume_it;

  error |= (price_it == prices.end());
  error |= (value < volume);
  CHECK_ERROR;

  value -= volume;
  if (UNLIKELY(value == 0))
  {
    prices.erase(price_it);
    volumes.erase(volume_it);
  }
}

HOT void OrderBook::executeOrder(const Side side, UNUSED const uint32_t price, uint64_t volume)
{
  const Side other_side = static_cast<Side>(side ^ 1);
  auto& volumes = volume_arrays[other_side];
  size_t size = volumes.size();

  while (volume & size)
  {
    uint64_t &available = volumes[size - 1];
    const uint64_t traded = std::min(available, volume);

    available -= traded;
    volume -= traded;
    size -= (available == 0);
  }

  volumes.resize(size);
}

template <typename Compare>
HOT std::vector<uint32_t>::const_iterator OrderBook::findPrice(const std::vector<uint32_t> &prices, const uint32_t price, Compare comp) const
{
  auto it = prices.crbegin();
  auto end = prices.crend();

  //TOOD find a way to parallelize, SIMD. very hot function
  while (it != end && comp(*it, price))
    ++it;

  return it.base();
}