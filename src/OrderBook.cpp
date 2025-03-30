/*================================================================================

File: OrderBook.cpp                                                             
Creator: Claudio Raimondi                                                       
Email: claudio.raimondi@pm.me                                                   

created at: 2025-03-07 21:17:51                                                 
last edited: 2025-03-30 18:58:16                                                

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

  orders.emplace(id, Order{price, qty});

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
  const Order &order = it->second;

  removeOrder(price_arrays[side], qty_arrays[side], order);
  orders.erase(it);
}

HOT void OrderBook::removeOrder(const uint64_t id, const Side side, const int32_t price, const uint64_t qty)
{
  const Order order = {price, qty};

  removeOrder(price_arrays[side], qty_arrays[side], order);
  orders.erase(id);
}

HOT void OrderBook::removeOrder(std::vector<int32_t> &prices, std::vector<uint64_t> &qtys, const Order &order)
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
  const size_t size = prices.size();
  const int32_t *data = prices.data();

  const int32_t *current = data + size - 1;

#if defined(__AVX512F__)

  const __m512i price_vec = _mm512_set1_epi32(price);

  while (current - data >= 16)
  {
    const uint8_t prefetch_offset = (current - data >= 32) * 32;
    PREFETCH_R(current - prefetch_offset, 0);

    const __m512i chunk = _mm512_loadu_si512(current - 16);
    __mmask16 mask;

    if constexpr (std::is_same_v<Compare, std::less<int32_t>>)
      mask = _mm512_cmplt_epi32_mask(chunk, price_vec);
    else if constexpr (std::is_same_v<Compare, std::greater<int32_t>>)
      mask = _mm512_cmpgt_epi32_mask(chunk, price_vec);
    else
      mask = _mm512_cmpeq_epi32_mask(chunk, price_vec);

    if (mask != 0xFFFF)
    {
      const uint32_t mismatch_pos = _tzcnt_u32(~mask);
      return prices.cbegin() + std::distance(data, current - 16 + mismatch_pos);
    }

    current -= 16;
  }

#elif defined(__AVX2__)

  const __m256i price_vec = _mm256_set1_epi32(price);

  while (current - data >= 8)
  {
    const uint8_t prefetch_offset = (current - data >= 16) * 16;
    PREFETCH_R(current - prefetch_offset, 0);

    const __m256i chunk = _mm256_loadu_si256(reinterpret_cast<const __m256i *>(current - 8));
    __m256i comp_result;

    if constexpr (std::is_same_v<Compare, std::less<int32_t>>)
      comp_result = _mm256_cmpgt_epi32(price_vec, chunk);
    else if constexpr (std::is_same_v<Compare, std::greater<int32_t>>)
      comp_result = _mm256_cmpgt_epi32(chunk, price_vec);
    else
      comp_result = _mm256_cmpeq_epi32(chunk, price_vec);

    uint64_t mask = _mm256_movemask_epi8(comp_result);
    mask = _pext_u64(mask, 0x1111111111111111);

    if (mask != 0xFF)
    {
      const uint32_t mismatch_pos = _tzcnt_u32(~mask);
      return prices.cbegin() + std::distance(data, current - 8 + mismatch_pos);
    }

    current -= 8;
  }

#endif

  while (current >= data)
  {
    if (comp(*current, price))
      return prices.cbegin() + std::distance(data, current);

    --current;
  }

  return prices.cend();
}