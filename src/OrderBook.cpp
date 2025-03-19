/*================================================================================

File: OrderBook.cpp                                                             
Creator: Claudio Raimondi                                                       
Email: claudio.raimondi@pm.me                                                   

created at: 2025-03-07 21:17:51                                                 
last edited: 2025-03-08 21:24:05                                                

================================================================================*/

#include <algorithm>

#include "OrderBook.hpp"
#include "macros.hpp"

OrderBook::~OrderBook(void) {}

HOT inline std::pair<uint32_t, uint32_t> OrderBook::getBestPrices(void) const
{
  return std::make_pair(price_arrays[BID].back(), price_arrays[ASK].back());
}

HOT inline void OrderBook::addOrder(const Side side, const uint32_t price, const uint64_t volume)
{
  std::vector<uint32_t> &prices = price_arrays[side];
  std::vector<uint64_t> &volumes = volume_arrays[side];

  constexpr bool (*comparators[])(uint32_t, uint32_t) = {
    [](uint32_t a, uint32_t b) -> bool { return a < b; },
    [](uint32_t a, uint32_t b) -> bool { return a > b; }
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

HOT inline void OrderBook::removeOrder(const Side side, const uint32_t price, const uint64_t volume)
{
  std::vector<uint32_t> &prices = price_arrays[side];
  std::vector<uint64_t> &volumes = volume_arrays[side];

  auto price_it = findPrice(prices, price, std::not_equal_to<uint32_t>());
  auto volume_it = volumes.begin() + std::distance(prices.cbegin(), price_it);

  *volume_it -= volume;
  if (UNLIKELY(*volume_it == 0))
  {
    prices.erase(price_it);
    volumes.erase(volume_it);
  }
}

HOT inline void OrderBook::executeOrder(const Side side, uint64_t volume)
{
  const Side other_side = static_cast<Side>(side ^ 1);
  auto& volumes = volume_arrays[other_side];
  size_t size = volumes.size();

  while (volume & size)
  {
    uint64_t& available = volumes[size - 1];
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

  //TODO SIMD (backwards)
  while (it != end && comp(*it, price))
    ++it;

  return it.base();
}