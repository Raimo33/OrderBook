/*================================================================================

File: OrderBook.cpp                                                             
Creator: Claudio Raimondi                                                       
Email: claudio.raimondi@pm.me                                                   

created at: 2025-03-07 21:17:51                                                 
last edited: 2025-04-10 21:32:16                                                

================================================================================*/

#include <ranges>

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
  bids.order_ids.emplace_back();
  asks.order_ids.emplace_back();
  bids.order_qtys.emplace_back();
  asks.order_qtys.emplace_back();
}

COLD OrderBook::OrderBook(OrderBook &&other) noexcept :
  bids(std::move(other.bids)),
  asks(std::move(other.asks)),
  equilibrium_price(other.equilibrium_price),
  equilibrium_bid_qty(other.equilibrium_bid_qty),
  equilibrium_ask_qty(other.equilibrium_ask_qty)
{
}

COLD OrderBook::~OrderBook()
{
}

HOT void OrderBook::addOrder(const uint64_t id, const Side side, const int32_t price, const uint64_t qty)
{
  using Handler = void (OrderBook::*)(const uint64_t, const int32_t, const uint64_t);
  static constexpr Handler handlers[] = {
    &OrderBook::addOrderBid,
    &OrderBook::addOrderAsk
  };

  (this->*handlers[side])(id, price, qty);
}

HOT ALWAYS_INLINE inline void OrderBook::addOrderBid(const uint64_t id, const int32_t price, const uint64_t qty)
{
  addOrder<std::less_equal<int32_t>>(bids, id, price, qty);
}

HOT ALWAYS_INLINE inline void OrderBook::addOrderAsk(const uint64_t id, const int32_t price, const uint64_t qty)
{
  addOrder<std::greater_equal<int32_t>>(asks, id, price, qty);
}

template<typename Comparator>
HOT void OrderBook::addOrder(PriceLevels &levels, const uint64_t id, const int32_t price, const uint64_t qty)
{
  const auto &prices = levels.prices;

  static constexpr Comparator cmp;
  ssize_t price_idx = utils::rfind(std::span{prices}, price, cmp);

  using Handler = void (OrderBook::*)(PriceLevels &, const size_t, const int32_t, const uint64_t, const uint64_t);
  static constexpr Handler handlers[] = {
    &OrderBook::addOrderToExistingPriceLevel,
    &OrderBook::addOrderToNewPriceLevel
  };

  const uint8_t handler_idx = (prices[price_idx] != price);
  (this->*handlers[handler_idx])(levels, price_idx, price, id, qty);
}

HOT void OrderBook::removeOrder(const uint64_t id, const Side side)
{
  using Handler = void (OrderBook::*)(const uint64_t);
  static constexpr Handler handlers[] = {
    &OrderBook::removeOrderBid,
    &OrderBook::removeOrderAsk
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
  for (auto order_ids_it = levels.order_ids.rbegin(); order_ids_it != levels.order_ids.rend(); ++order_ids_it)
  {
    auto &order_ids = *order_ids_it;

    static constexpr std::equal_to<uint64_t> order_ids_cmp;
    const ssize_t order_idx = utils::rfind(std::span<const uint64_t>{order_ids}, id, order_ids_cmp);
    if (order_idx == -1) [[likely]]
      continue;

    const size_t price_idx = std::distance(levels.order_ids.begin(), order_ids_it.base()) - 1;

    auto &cumulative_qty = levels.cumulative_qtys[price_idx];
    auto &order_qtys = levels.order_qtys[price_idx];

    const uint64_t qty = order_qtys[order_idx];

    cumulative_qty -= qty;
    if (cumulative_qty > 0) [[likely]]
    {
      order_ids[order_idx] = order_ids.back();
      order_qtys[order_idx] = order_qtys.back();
      order_ids.pop_back();
      order_qtys.pop_back();
    }
    else
      removePriceLevel(levels, price_idx, 0);
  }
}

HOT void OrderBook::removeOrder(const uint64_t id, const Side side, const int32_t price, const uint64_t qty)
{
  using Handler = void (OrderBook::*)(const uint64_t, const int32_t, const uint64_t);
  static constexpr Handler handlers[] = {
    &OrderBook::removeOrderBid,
    &OrderBook::removeOrderAsk
  };

  (this->*handlers[side])(id, price, qty);
}

HOT ALWAYS_INLINE inline void OrderBook::removeOrderBid(const uint64_t id, const int32_t price, const uint64_t qty)
{
  removeOrder<std::less_equal<int32_t>>(bids, id, price, qty);
}

HOT ALWAYS_INLINE inline void OrderBook::removeOrderAsk(const uint64_t id, const int32_t price, const uint64_t qty)
{
  removeOrder<std::greater_equal<int32_t>>(asks, id, price, qty);
}

template<typename Comparator>
HOT void OrderBook::removeOrder(PriceLevels &levels, const uint64_t id, const int32_t price, const uint64_t qty)
{
  static constexpr Comparator prices_cmp;
  const size_t price_idx = utils::rfind(std::span<const int32_t>{levels.prices}, price, prices_cmp);

  auto &cumulative_qty = levels.cumulative_qtys[price_idx];
  cumulative_qty -= qty;

  if (cumulative_qty > 0) [[likely]]
    removeOrderFromPriceLevel(levels, price_idx, id);
  else
    removePriceLevel(levels, price_idx, 0);
}

HOT void OrderBook::executeOrder(const uint64_t id, const Side side, const uint64_t qty)
{
  using Handler = void (OrderBook::*)(const uint64_t, uint64_t);
  static constexpr Handler handlers[] = {
    &OrderBook::executeOrderBid,
    &OrderBook::executeOrderAsk
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
  const size_t price_idx = levels.prices.size() - 1;

  auto &cumulative_qty = levels.cumulative_qtys.back();
  cumulative_qty -= qty;

  using Handler = void (OrderBook::*)(PriceLevels &, const size_t, const uint64_t);
  static constexpr Handler handlers[] = {
    &OrderBook::removeOrderFromPriceLevel,
    &OrderBook::removePriceLevel
  };

  const uint8_t idx = (cumulative_qty == 0);
  (this->*handlers[idx])(levels, price_idx, id);
}

HOT void OrderBook::addOrderToExistingPriceLevel(PriceLevels &levels, const size_t price_idx, UNUSED const int32_t price, const uint64_t id, const uint64_t qty)
{
  auto &cumulative_qtys = levels.cumulative_qtys;
  auto &order_ids = levels.order_ids;
  auto &order_qtys = levels.order_qtys;

  cumulative_qtys[price_idx] += qty;
  order_ids[price_idx].push_back(id);
  order_qtys[price_idx].push_back(qty);
}

HOT void OrderBook::addOrderToNewPriceLevel(PriceLevels &levels, const size_t price_idx, const int32_t price, const uint64_t id, const uint64_t qty)
{
  auto &prices = levels.prices;
  auto &cumulative_qtys = levels.cumulative_qtys;
  auto &order_ids = levels.order_ids;
  auto &order_qtys = levels.order_qtys;

  const size_t new_price_idx = price_idx + 1;

  prices.insert(prices.cbegin() + new_price_idx, price);
  cumulative_qtys.insert(cumulative_qtys.cbegin() + new_price_idx, qty);
  order_ids.emplace(order_ids.cbegin() + new_price_idx, std::vector<uint64_t>{id});
  order_qtys.emplace(order_qtys.cbegin() + new_price_idx, std::vector<uint64_t>{qty});
}

HOT void OrderBook::removeOrderFromPriceLevel(PriceLevels &levels, const size_t price_idx, const uint64_t id)
{
  auto &order_ids = levels.order_ids[price_idx];
  auto &order_qtys = levels.order_qtys[price_idx];

  static constexpr std::equal_to<uint64_t> order_ids_cmp;
  const ssize_t order_idx = utils::rfind(std::span<const uint64_t>{order_ids}, id, order_ids_cmp);

  order_ids[order_idx] = order_ids.back();
  order_qtys[order_idx] = order_qtys.back();
  order_ids.pop_back();
  order_qtys.pop_back();
}

HOT void OrderBook::removePriceLevel(PriceLevels &levels, const size_t price_idx, UNUSED const uint64_t id)
{
  auto &prices = levels.prices;
  auto &cumulative_qtys = levels.cumulative_qtys;
  auto &order_ids = levels.order_ids;
  auto &order_qtys = levels.order_qtys;

  prices.erase(prices.cbegin() + price_idx);
  cumulative_qtys.erase(cumulative_qtys.cbegin() + price_idx);
  order_ids.erase(order_ids.cbegin() + price_idx);
  order_qtys.erase(order_qtys.cbegin() + price_idx);
}


