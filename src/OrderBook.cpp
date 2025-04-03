/*================================================================================

File: OrderBook.cpp                                                             
Creator: Claudio Raimondi                                                       
Email: claudio.raimondi@pm.me                                                   

created at: 2025-03-07 21:17:51                                                 
last edited: 2025-04-02 21:57:52                                                

================================================================================*/

#include <immintrin.h>
#include <variant>

#include "OrderBook.hpp"
#include "macros.hpp"
#include "error.hpp"

extern volatile bool error;

COLD OrderBook::OrderBook(const uint64_t id) noexcept :
  id(id),
  equilibrium_price(INT32_MIN),
  equilibrium_bid_qty(INT32_MIN)
{
  bids.prices.push_back(INT32_MIN);
  asks.prices.push_back(INT32_MAX);
  bids.cumulative_qtys.push_back(0);
  asks.cumulative_qtys.push_back(0);
}

HOT void OrderBook::addOrder(const uint64_t id, const Side side, const int32_t price, const uint64_t qty)
{
  //TODO
}

HOT void OrderBook::removeOrder(const uint64_t id, const Side side)
{
  //TODO
}

HOT void OrderBook::removeOrder(const uint64_t id, const Side side, const int32_t price, const uint64_t qty)
{
  //TODO
}

HOT void OrderBook::executeOrder(const uint64_t id, const Side side, uint64_t qty)
{
  //TODO
}

//TODO avx now only works for signed 32bit ints
template <typename T, typename Comparator = std::equal_to<T>>
HOT static typename std::vector<T>::const_iterator find(const std::vector<T> &array, const T elem)
{
  constexpr Comparator comp;
  const T *start = array.data();
  size_t remaining = array.size();
  const T *current = start + remaining - 1;
  bool keep_looking = true;

  PREFETCH_R(current, 0);
  const bool safe = (remaining >= chunk_size);
  PREFETCH_R(current - chunk_size * safe, 0);

  uint8_t misalignment = misalignment_backwards(current, CACHELINE_SIZE);
  misalignment -= (misalignment > remaining) * (misalignment - remaining);
  remaining -= misalignment;

  keep_looking = (misalignment > 0);
  while (keep_looking)
  {
    keep_looking = comp(elem, *current);

    --current;
    --misalignment;

    keep_looking &= (misalignment > 0);
  }

  if (LIKELY(misalignment > 0))
    return array.cbegin() + misalignment;

#if defined(__AVX512F__)
  const __m512i elem_vec = _mm512_set1_epi32(elem);
  __mmask16 mask = 0xFFFF;

  keep_looking = (remaining >= chunk_size);
  while (keep_looking)
  {
    const __m512i chunk = _mm512_load_si512(current - chunk_size);

    const bool safe = (remaining >= (chunk_size * 2));
    PREFETCH_R(current - chunk_size * safe, 0);

    if constexpr (std::is_same_v<Compare, std::less<T>>)
      mask = _mm512_cmplt_epi32_mask(chunk, elem_vec);
    else if constexpr (std::is_same_v<Compare, std::greater<T>>)
      mask = _mm512_cmpgt_epi32_mask(chunk, elem_vec);
    else if constexpr (std::is_same_v<Compare, std::equal_to<T>>)
      mask = _mm512_cmpeq_epi32_mask(chunk, elem_vec);
    else
      static_assert(false, "Unsupported comparison type");

    current -= chunk_size;
    remaining -= chunk_size;

    keep_looking = (remaining >= chunk_size);
    keep_looking &= (mask == 0xFFFF);
  }

  if (LIKELY(mask != 0xFFFF))
  {
    const uint32_t mismatch_pos = _tzcnt_u32(~mask);
    return array.cbegin() + (remaining + mismatch_pos);
  }
#endif

  PREFETCH_R(start, 0);

  keep_looking = (remaining > 0);
  while (keep_looking)
  {
    keep_looking = comp(elem, *current);

    --current;
    --remaining;

    keep_looking &= (remaining > 0);
  }

  if (LIKELY(remaining > 0))
    return array.cbegin() + remaining;

  return array.cend();
}
