/*================================================================================

File: OrderBook.cpp                                                             
Creator: Claudio Raimondi                                                       
Email: claudio.raimondi@pm.me                                                   

created at: 2025-03-07 21:17:51                                                 
last edited: 2025-03-31 15:01:09                                                

================================================================================*/

#include <immintrin.h>

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

HOT void OrderBook::addOrder(const uint64_t id, const Side side, const int32_t price, const uint64_t qty)
{
  std::vector<int32_t> &prices = price_arrays[side];
  std::vector<uint64_t> &qtys = qty_arrays[side];

  orders.emplace(id, BookEntry{price, qty});

  constexpr bool (*comparators[])(const int32_t, const int32_t) = {
    [](const int32_t a, const int32_t b) { return a < b; },
    [](const int32_t a, const int32_t b) { return a > b; }
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
  const auto price_it = findPrice(prices, order.price, std::not_equal_to<int32_t>());
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
  std::vector<int32_t> &prices = price_arrays[side];
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
HOT std::vector<int32_t>::const_iterator OrderBook::findPrice(const std::vector<int32_t> &prices, const int32_t price, Compare comp) const
{
  const int32_t *start = prices.data();
  size_t remaining = prices.size();
  const int32_t *current = start + remaining - 1;

  static_assert(CACHELINE_SIZE <= INT8_MAX, "CACHELINE_SIZE too large");
  static_assert(CACHELINE_SIZE % sizeof(int32_t) == 0, "CACHELINE_SIZE must be a multiple of sizeof(int32_t)");
  static_assert(std::endian::native == std::endian::little, "Endianess not supported");
  static_assert(std::is_same_v<Compare, std::less<int32_t>> || std::is_same_v<Compare, std::greater<int32_t>> || std::is_same_v<Compare, std::equal_to<int32_t>>, "Unsupported comparator");

  uint8_t misalignment = misalignment_backwards(current, CACHELINE_SIZE);
  misalignment -= (misalignment > remaining) * (misalignment - remaining);

  while (misalignment--)
  {
    if (comp(*current, price))
      return current;

    --current;
    --remaining;
  }

#if defined(__AVX512F__)

  //TODO rename prefetch_counter, simplify logic

  const __m512i price_vec = _mm512_set1_epi32(price);
  constexpr int32_t prefetch_distance = CACHELINE_SIZE / sizeof(price);
  uint8_t prefetch_counter = prefetch_distance;

  while (remaining >= 16)
  {
    const __m512i chunk = _mm512_load_si512(current - 16);
    __mmask16 mask;

    if (iteration_count == prefetch_distance)
    {
      PREFETCH_R(current - prefetch_distance, 1);
      prefetch_counter = 16;
    }

    if constexpr (std::is_same_v<Compare, std::less<int32_t>>)
      mask = _mm512_cmplt_epi32_mask(chunk, price_vec);
    else if constexpr (std::is_same_v<Compare, std::greater<int32_t>>)
      mask = _mm512_cmpgt_epi32_mask(chunk, price_vec);
    else
      mask = _mm512_cmpeq_epi32_mask(chunk, price_vec);

    if (mask != 0xFFFF)
    {
      const uint32_t mismatch_pos = _tzcnt_u32(~mask);
      return current - 16 + mismatch_pos;
    }

    current -= 16;
    remaining -= 16;
    prefetch_counter += 16;
  }

#endif

  while (remaining)
  {
    if (comp(*current, price))
      return current;

    --current;
    --remaining;
  }

  return prices.cend();
}