/*================================================================================

File: OrderBook.cpp                                                             
Creator: Claudio Raimondi                                                       
Email: claudio.raimondi@pm.me                                                   

created at: 2025-03-07 21:17:51                                                 
last edited: 2025-04-01 19:30:34                                                

================================================================================*/

#include <immintrin.h>
#include <variant>

#include "OrderBook.hpp"
#include "macros.hpp"
#include "error.hpp"

extern volatile bool error;

COLD OrderBook::OrderBook(const uint32_t id, const std::string_view symbol) noexcept :
  id(id),
  symbol(symbol),
  equilibrium_price(INT32_MIN),
  equilibrium_bid_qty(INT32_MIN)
{
  price_arrays[BID].push_back(0);
  price_arrays[ASK].push_back(UINT32_MAX);
  qty_arrays[BID].push_back(0);
  qty_arrays[ASK].push_back(UINT64_MAX);
}

COLD OrderBook::~OrderBook(void) {}

HOT void OrderBook::addOrder(const uint64_t id, const Side side, const int32_t price, const uint64_t qty)
{
  std::vector<int32_t> &prices = price_arrays[side];
  std::vector<uint64_t> &qtys = qty_arrays[side];

  orders.emplace(id, BookEntry{price, qty});

  using FindPriceFunc = std::vector<int32_t>::const_iterator (OrderBook::*)(const std::vector<int32_t>&, const int32_t) const;
  static const FindPriceFunc findPrice_funcs[] = {
    &OrderBook::findPrice<std::less<int32_t>>,
    &OrderBook::findPrice<std::greater<int32_t>>
  };

  const auto price_it = (this->*findPrice_funcs[side])(prices, price);
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
  const auto it = orders.find(id);
  const BookEntry &order = it->second;

  removeOrder(price_arrays[side], qty_arrays[side], order);
  orders.erase(it);
}

HOT void OrderBook::removeOrder(const uint64_t id, const Side side, const int32_t price, const uint64_t qty)
{
  const BookEntry order = {price, qty};

  removeOrder(price_arrays[side], qty_arrays[side], order);
  orders.erase(id);
}

HOT void OrderBook::removeOrder(std::vector<int32_t> &prices, std::vector<uint64_t> &qtys, const BookEntry &order)
{
  const auto price_it = findPrice<std::equal_to<int32_t>>(prices, order.price);
  const auto qty_it = qtys.begin() + std::distance(prices.cbegin(), price_it);

  auto &available_qty = *qty_it;
  available_qty -= order.qty;

  using LevelRemover = void(*)(std::vector<int32_t>&, std::vector<uint64_t>&, std::vector<int32_t>::const_iterator, std::vector<uint64_t>::iterator);
  constexpr LevelRemover removeLevel[] = {
    [](std::vector<int32_t>&, std::vector<uint64_t>&, auto, auto) {},
    [](std::vector<int32_t>& prices, std::vector<uint64_t>& qtys, auto price_it, auto qty_it) {
      prices.erase(price_it);
      qtys.erase(qty_it);
    }
  };

  removeLevel[available_qty == 0](prices, qtys, price_it, qty_it);
}

HOT void OrderBook::executeOrder(const uint64_t id, const Side side, uint64_t qty)
{
  std::vector<int32_t> &prices = price_arrays[side];
  std::vector<uint64_t> &qtys = qty_arrays[side];

  orders.erase(id);

  auto &available_qty = qtys.back();
  available_qty -= qty;

  using OrderRemover = void(*)(std::vector<int32_t>&, std::vector<uint64_t>&);
  constexpr OrderRemover removeOrder[] = {
    [](std::vector<int32_t> &, std::vector<uint64_t> &) {},
    [](std::vector<int32_t> &prices, std::vector<uint64_t> &qtys) {
      prices.pop_back();
      qtys.pop_back();
    }
  };

  removeOrder[available_qty == 0](prices, qtys);
}

template <typename Compare>
HOT std::vector<int32_t>::const_iterator OrderBook::findPrice(const std::vector<int32_t> &prices, const int32_t price) const
{
  const int32_t *start = prices.data();
  size_t remaining = prices.size();
  const int32_t *current = start + remaining - 1;
  constexpr Compare comp;
  bool keep_looking = true;

  PREFETCH_R(current, 0);
  const bool safe = (remaining >= 16);
  PREFETCH_R(current - 16 * safe, 0);

  static_assert(CACHELINE_SIZE / sizeof(price) == 16, "unexpected CACHELINE_SIZE, prefetching not synchronized");
  static_assert(std::endian::native == std::endian::little, "Endianess not supported");

  uint8_t misalignment = misalignment_backwards(current, CACHELINE_SIZE);
  misalignment -= (misalignment > remaining) * (misalignment - remaining);
  remaining -= misalignment;

  keep_looking = (misalignment > 0);
  while (keep_looking)
  {
    keep_looking = comp(price, *current);

    --current;
    --misalignment;

    keep_looking &= (misalignment > 0);
  }

  if (LIKELY(misalignment > 0))
    return prices.cbegin() + misalignment;

#if defined(__AVX512F__)
  const __m512i price_vec = _mm512_set1_epi32(price);
  __mmask16 mask = 0xFFFF;

  keep_looking = (remaining >= 16);
  while (keep_looking)
  {
    const __m512i chunk = _mm512_load_si512(current - 16);

    const bool safe = (remaining >= 32);
    PREFETCH_R(current - 32 * safe, 0);

    if constexpr (std::is_same_v<Compare, std::less<int32_t>>)
      mask = _mm512_cmplt_epi32_mask(chunk, price_vec);
    else if constexpr (std::is_same_v<Compare, std::greater<int32_t>>)
      mask = _mm512_cmpgt_epi32_mask(chunk, price_vec);
    else if constexpr (std::is_same_v<Compare, std::equal_to<int32_t>>)
      mask = _mm512_cmpneq_epi32_mask(chunk, price_vec);
    else
      static_assert(false, "Unsupported comparison type");

    current -= 16;
    remaining -= 16;

    keep_looking = (remaining >= 16);
    keep_looking &= (mask == 0xFFFF);
  }

  if (LIKELY(mask != 0xFFFF))
  {
    const uint32_t mismatch_pos = _tzcnt_u32(~mask);
    return prices.cbegin() + (remaining + mismatch_pos);
  }
#endif

  PREFETCH_R(start, 0);

  keep_looking = (remaining > 0);
  while (keep_looking)
  {
    keep_looking = comp(price, *current);

    --current;
    --remaining;

    keep_looking &= (remaining > 0);
  }

  if (LIKELY(remaining > 0))
    return prices.cbegin() + remaining;

  return prices.cend();
}
