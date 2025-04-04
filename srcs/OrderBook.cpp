/*================================================================================

File: OrderBook.cpp                                                             
Creator: Claudio Raimondi                                                       
Email: claudio.raimondi@pm.me                                                   

created at: 2025-03-07 21:17:51                                                 
last edited: 2025-04-05 00:18:43                                                

================================================================================*/

#include "OrderBook.hpp"
#include "utils/utils.hpp"
#include "macros.hpp"
#include "error.hpp"

extern volatile bool error;

COLD OrderBook::OrderBook(void) noexcept :
  equilibrium_price(INT32_MIN),
  equilibrium_bid_qty(0),
  equilibrium_ask_qty(0)
{
  bids.prices.push_back(INT32_MIN);
  asks.prices.push_back(INT32_MAX);
  bids.cumulative_qtys.push_back(0);
  asks.cumulative_qtys.push_back(0);
}

COLD OrderBook::OrderBook(OrderBook &&other) noexcept :
  bids(std::move(other.bids)),
  asks(std::move(other.asks)),
  equilibrium_price(other.equilibrium_price),
  equilibrium_bid_qty(other.equilibrium_bid_qty),
  equilibrium_ask_qty(other.equilibrium_ask_qty)
{
}

COLD OrderBook::~OrderBook(void)
{
}

HOT void OrderBook::addOrder(const uint64_t id, const Side side, const int32_t price, const uint64_t qty)
{
  using Handler = void (OrderBook::*)(const uint64_t, const int32_t, const uint64_t);
  static const Handler handlers[] = {
    [BID] = &OrderBook::addOrderBid,
    [ASK] = &OrderBook::addOrderAsk
  };

  (this->*handlers[side])(id, price, qty);
}

HOT ALWAYS_INLINE inline void OrderBook::addOrderBid(const uint64_t id, const int32_t price, const uint64_t qty)
{
  addOrder<std::greater_equal<int32_t>>(bids, id, price, qty);
}

HOT ALWAYS_INLINE inline void OrderBook::addOrderAsk(const uint64_t id, const int32_t price, const uint64_t qty)
{
  addOrder<std::less_equal<int32_t>>(asks, id, price, qty);
}

template<typename Comparator>
HOT void OrderBook::addOrder(PriceLevels &levels, const uint64_t id, const int32_t price, const uint64_t qty)
{
  auto &prices = levels.prices;
  auto &cumulative_qtys = levels.cumulative_qtys;
  auto &order_ids = levels.order_ids;
  auto &order_qtys = levels.order_qtys;

  static constexpr Comparator cmp;
  const auto prices_it = utils::rfind(prices, price, cmp);

  const size_t index = std::distance(prices.cbegin(), prices_it);

  if (LIKELY(prices_it != prices.cend() && *prices_it == price))
  {
    cumulative_qtys[index] += qty;
    order_ids[index].push_back(id);
    order_qtys[index].push_back(qty);
  }
  else
  {
    const auto cumulative_qty_it = cumulative_qtys.begin() + index;
    const auto order_ids_it = order_ids.begin() + index;
    const auto order_qtys_it = order_qtys.begin() + index;

    prices.insert(prices_it, price);
    cumulative_qtys.insert(cumulative_qty_it, qty);
    order_ids.insert(order_ids_it, {id});
    order_qtys.insert(order_qtys_it, {qty});
  }
}

HOT void OrderBook::removeOrder(const uint64_t id, const Side side)
{
  using Handler = void (OrderBook::*)(const uint64_t);
  static const Handler handlers[] = {
    [BID] = &OrderBook::removeOrderBid,
    [ASK] = &OrderBook::removeOrderAsk
  };

  (this->*handlers[side])(id);
}

HOT ALWAYS_INLINE inline void OrderBook::removeOrderBid(const uint64_t id)
{
  removeOrder(bids, id);
}

HOT ALWAYS_INLINE inline void OrderBook::removeOrderAsk(const uint64_t id)
{
  removeOrder(asks, id);
}

HOT void OrderBook::removeOrder(PriceLevels &levels, const uint64_t id)
{
  for (auto &order_ids : levels.order_ids | std::views::reverse)
  {
    static constexpr std::equal_to<uint64_t> order_ids_cmp;
    const auto order_ids_it = utils::rfind(order_ids, id, order_ids_cmp);

    if (LIKELY(order_ids_it == order_ids.cend()))
      continue;
    
    const size_t price_index = std::distance(levels.order_ids.cbegin(), order_ids);
    const size_t order_index = std::distance(order_ids.cbegin(), order_ids_it);

    auto &order_qtys = levels.order_qtys[price_index];

    const uint64_t qty = order_qtys[order_index];
    auto &cumulative_qty = levels.cumulative_qtys[price_index];

    cumulative_qty -= qty;
    if (LIKELY(cumulative_qty > 0))
    {
      std::swap(order_ids[order_index], order_ids.back());
      std::swap(order_qtys[order_index], order_qtys.back());
      order_ids.pop_back();
      order_qtys.pop_back();
    }
    else
    {
      levels.prices.erase(levels.prices.cbegin() + price_index);
      levels.cumulative_qtys.erase(levels.cumulative_qtys.cbegin() + price_index);
      levels.order_ids.erase(levels.order_ids.cbegin() + price_index);
      levels.order_qtys.erase(levels.order_qtys.cbegin() + price_index);
    }
  }


}

HOT void OrderBook::removeOrder(const uint64_t id, const Side side, const int32_t price, const uint64_t qty)
{
  using Handler = void (OrderBook::*)(const uint64_t, const int32_t, const uint64_t);
  static const Handler handlers[] = {
    [BID] = &OrderBook::removeOrderBid,
    [ASK] = &OrderBook::removeOrderAsk
  };

  (this->*handlers[side])(id, price, qty);
}

HOT ALWAYS_INLINE inline void OrderBook::removeOrderBid(const uint64_t id, const int32_t price, const uint64_t qty)
{
  removeOrder<std::greater_equal<int32_t>>(bids, id, price, qty);
}

HOT ALWAYS_INLINE inline void OrderBook::removeOrderAsk(const uint64_t id, const int32_t price, const uint64_t qty)
{
  removeOrder<std::less_equal<int32_t>>(asks, id, price, qty);
}

template<typename Comparator>
HOT void OrderBook::removeOrder(PriceLevels &levels, const uint64_t id, const int32_t price, const uint64_t qty)
{
  auto &prices = levels.prices;
  auto &cumulative_qtys = levels.cumulative_qtys;

  static constexpr Comparator prices_cmp;
  const auto prices_it = utils::rfind(prices, price, prices_cmp);
  const size_t price_index = std::distance(prices.cbegin(), prices_it);

  auto &cumulative_qty = cumulative_qtys[price_index];
  cumulative_qty -= qty;

  if (LIKELY(cumulative_qty > 0))
  {
    auto &order_ids = levels.order_ids[price_index];
    auto &order_qtys = levels.order_qtys[price_index];
  
    static constexpr std::equal_to<uint64_t> order_ids_cmp;
    const auto order_ids_it = utils::rfind(order_ids, id, order_ids_cmp);
    const size_t order_index = std::distance(order_ids.cbegin(), order_ids_it);
  
    std::swap(order_ids[order_index], order_ids.back());
    std::swap(order_qtys[order_index], order_qtys.back());
    order_ids.pop_back();
    order_qtys.pop_back();
  }
  else
  {
    prices.erase(prices_it);
    cumulative_qtys.erase(cumulative_qtys.cbegin() + price_index);
    levels.order_ids.erase(levels.order_ids.cbegin() + price_index);
    levels.order_qtys.erase(levels.order_qtys.cbegin() + price_index);
  }
}

HOT void OrderBook::executeOrder(const uint64_t id, const Side side, const uint64_t qty)
{
  using Handler = void (OrderBook::*)(const uint64_t, uint64_t);
  static const Handler handlers[] = {
    [BID] = &OrderBook::executeOrderBid,
    [ASK] = &OrderBook::executeOrderAsk
  };

  (this->*handlers[side])(id, qty);
}

HOT ALWAYS_INLINE inline void OrderBook::executeOrderBid(const uint64_t id, const uint64_t qty)
{
  executeOrder(bids, id, qty);
}

HOT ALWAYS_INLINE inline void OrderBook::executeOrderAsk(const uint64_t id, const uint64_t qty)
{
  executeOrder(asks, id, qty);
}

HOT void OrderBook::executeOrder(PriceLevels &levels, const uint64_t id, const uint64_t qty)
{
  auto &cumulative_qty = levels.cumulative_qtys.back();

  cumulative_qty -= qty;

  if (LIKELY(cumulative_qty > 0))
  {
    auto &order_ids = levels.order_ids.back();
    auto &order_qtys = levels.order_qtys.back();

    static constexpr std::equal_to<uint64_t> order_ids_cmp;
    const auto order_ids_it = utils::rfind(order_ids, id, order_ids_cmp);
    const size_t order_index = std::distance(order_ids.cbegin(), order_ids_it);

    std::swap(order_ids[order_index], order_ids.back());
    std::swap(order_qtys[order_index], order_qtys.back());
    order_ids.pop_back();
    order_qtys.pop_back();
  }
  else
  {
    levels.prices.pop_back();
    levels.cumulative_qtys.pop_back();
    levels.order_ids.pop_back();
    levels.order_qtys.pop_back();
  }
}


