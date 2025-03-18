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

HOT std::pair<uint32_t, uint32_t> OrderBook::getBestPrices(void) const
{
  static const auto &bids = price_arrays[BID];
  static const auto &asks = price_arrays[ASK];

  return std::make_pair(bids.back(), asks.back());
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

//TODO UB if the order is not in the book or the volume is greater than the available volume
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
  auto &volumes = volume_arrays[other_side];
  
  while (volume > 0 && !volumes.empty())
  {
    uint64_t &available_volume = volumes.back();
    uint64_t traded = std::min(volume, available_volume);

    available_volume -= traded;
    volume -= traded;

    if (UNLIKELY(available_volume == 0))
      volumes.pop_back();
  }
}

template <typename Compare>
HOT std::vector<uint32_t>::const_iterator OrderBook::findPrice(const std::vector<uint32_t> &prices, const uint32_t price, Compare comp) const
{
  auto it = prices.crbegin();
  auto end = prices.crend();

  //TODO SIMD
  while (it != end && comp(*it, price))
    ++it;

  return it.base();
}