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

OrderBook::OrderBook(void)
{
  comparators[BID] = [](const PriceLevel &lhs, const int32_t price) { return lhs.first < price; };
  comparators[ASK] = [](const PriceLevel &lhs, const int32_t price) { return lhs.first > price; };
}

OrderBook::~OrderBook(void) {}

HOT inline std::pair<uint32_t, uint32_t> OrderBook::getBestPrices(void) const
{
  return std::make_pair(level_arrays[BID].back().first, level_arrays[ASK].back().first);
}

HOT inline void OrderBook::addOrder(const Side side, const int32_t price, const uint64_t volume)
{
  addOrder(level_arrays[side], price, volume, comparators[side]);
}

HOT inline void OrderBook::removeOrder(const Side side, const int32_t price, const uint64_t volume)
{
  removeOrder(level_arrays[side], price, volume, comparators[side]);
}

HOT inline void OrderBook::executeOrder(const Side side, uint64_t volume)
{
  auto &levels = level_arrays[side];  //TODO flip side?
  
  while (volume > 0 && !levels.empty())
  {
    auto &level = levels.back();
    uint64_t traded = std::min(volume, level.second);

    level.second -= traded;
    volume -= traded;

    if (UNLIKELY(level.second <= 0))
      levels.pop_back();
  }
}

template <class Compare>
HOT void OrderBook::addOrder(std::vector<PriceLevel> &levels, const int32_t price, const uint64_t volume, Compare comp)
{
  auto it = std::lower_bound(levels.begin(), levels.end(), price, comp);

  if (LIKELY(it != levels.end() && it->first == price))
    it->second += volume;
  else
    levels.insert(it, std::make_pair(price, volume));
}

template <class Compare>
HOT void OrderBook::removeOrder(std::vector<PriceLevel> &levels, const int32_t price, const uint64_t volume, Compare comp)
{
  auto it = std::lower_bound(levels.begin(), levels.end(), price, comp);

  it->second -= volume;
  if (UNLIKELY(it->second <= 0))
    levels.erase(it);
}