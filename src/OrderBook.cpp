/*================================================================================

File: OrderBook.cpp                                                             
Creator: Claudio Raimondi                                                       
Email: claudio.raimondi@pm.me                                                   

created at: 2025-03-07 21:17:51                                                 
last edited: 2025-04-03 16:38:05                                                

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

template <typename T, typename Comparator>
HOT static typename std::vector<T>::const_iterator rfind(const std::vector<T> &array, const T elem, const Comparator &comp)
{
  static_assert(std::is_integral<T>::value, "T must be an integral type");
  static_assert(std::hardware_constructive_interference_size == 64, "Cache line size must be 64 bytes");

  constexpr uint8_t chunk_size = sizeof(__m512i) / sizeof(T);

  const T *begin = array.data();
  size_t remaining = array.size();
  const T *it = begin + remaining;
  bool keep_looking = true;

  PREFETCH_R(it, 0);
  const bool safe = (remaining >= chunk_size);
  PREFETCH_R(it - chunk_size * safe, 0);

  uint8_t misalignment = misalignment_backwards(it, 64);
  misalignment -= (misalignment > remaining) * (misalignment - remaining);
  remaining -= misalignment;

  keep_looking = (misalignment > 0);
  while (keep_looking)
  {
    --it;
    --misalignment;

    keep_looking = comp(elem, *it);
    keep_looking &= (misalignment > 0);
  }

  if (LIKELY(misalignment))
    return it;

#if defined(__AVX512F__)
  const __m512i elem_vec = [elem]() -> __m512i
  {
    if constexpr (sizeof(T) == sizeof(int8_t))
      return _mm512_set1_epi8((elem));
    else if constexpr (sizeof(T) == sizeof(int16_t))
      return _mm512_set1_epi16(elem);
    else if constexpr (sizeof(T) == sizeof(int32_t))
      return _mm512_set1_epi32(elem);
    else if constexpr (sizeof(T) == sizeof(int64_t))
      return _mm512_set1_epi64(elem);
    else
      static_assert(false, "Unsupported type");
  }();
  constexpr int opcode = []() -> int
  {
    if constexpr (std::is_same_v<Comparator, std::equal_to<T>>)
      return _MM_CMPINT_EQ;
    else if constexpr (std::is_same_v<Comparator, std::not_equal_to<T>>)
      return _MM_CMPINT_NE;
    else if constexpr (std::is_same_v<Comparator, std::less_equal<T>>)
      return _MM_CMPINT_LE;
    else if constexpr (std::is_same_v<Comparator, std::greater_equal<T>>)
      return _MM_CMPINT_GE;
    else if constexpr (std::is_same_v<Comparator, std::less<T>>)
      return _MM_CMPINT_LT;
    else if constexpr (std::is_same_v<Comparator, std::greater<T>>)
      return _MM_CMPINT_GT;
    else
      static_assert(false, "Unsupported comparator");
  }();
  __mmask64 mask = 0;

  keep_looking = (remaining >= chunk_size);
  while (keep_looking)
  {
    it -= chunk_size;
    const __m512i chunk = _mm512_loadu_si512(it);

    remaining -= chunk_size;
    keep_looking = (remaining >= chunk_size);

    PREFETCH_R(it - chunk_size * keep_looking, 0);

    if constexpr (std::is_signed_v<T>)
    {
      if constexpr (sizeof(T) == sizeof(int8_t))
        mask = _mm512_cmp_epi8_mask(chunk, elem_vec, opcode);
      else if constexpr (sizeof(T) == sizeof(int16_t))
        mask = _mm512_cmp_epi16_mask(chunk, elem_vec, opcode);
      else if constexpr (sizeof(T) == sizeof(int32_t))
        mask = _mm512_cmp_epi32_mask(chunk, elem_vec, opcode);
      else if constexpr (sizeof(T) == sizeof(int64_t))
        mask = _mm512_cmp_epi64_mask(chunk, elem_vec, opcode);
    }
    else
    {
      if constexpr (sizeof(T) == siezof(int8_t))
        mask = _mm512_cmp_epu8_mask(chunk, elem_vec, opcode);
      else if constexpr (sizeof(T) == sizeof(int16_t))
        mask = _mm512_cmp_epu16_mask(chunk, elem_vec, opcode);
      else if constexpr (sizeof(T) == sizeof(int32_t))
        mask = _mm512_cmp_epu32_mask(chunk, elem_vec, opcode);
      else if constexpr (sizeof(T) == sizeof(int64_t))
        mask = _mm512_cmp_epu64_mask(chunk, elem_vec, opcode);
    }

    keep_looking &= !mask;
  }

  if (LIKELY(mask))
  {
    const uint8_t matched_idx = 63 - __builtin_ctzll(mask);
    return it + matched_idx;
  }
#endif

  keep_looking = (remaining > 0);
  while (keep_looking)
  {
    --it;
    --remaining;
    
    keep_looking = comp(elem, *it);
    keep_looking &= (remaining > 0);
  }

  return it * (remaining > 0);
}
