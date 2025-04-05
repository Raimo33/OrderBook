/*================================================================================

File: utils.cpp                                                                 
Creator: Claudio Raimondi                                                       
Email: claudio.raimondi@pm.me                                                   

created at: 2025-04-03 20:16:29                                                 
last edited: 2025-04-05 11:22:59                                                

================================================================================*/

#pragma once

#include <cstdint>
#include <span>
#include <immintrin.h>

#include "utils.hpp"
#include "macros.hpp"

namespace utils
{

template <typename T, typename Comparator>
HOT ssize_t find(std::span<const T> data, const T &elem, const Comparator &comp) noexcept
{
  static_assert(std::is_integral<T>::value, "T must be an integral type");
  static_assert(std::hardware_constructive_interference_size == 64, "Cache line size must be 64 bytes");

  constexpr uint8_t chunk_size = sizeof(__m512i) / sizeof(T);

  const T *begin = data.data();
  const size_t size = data.size();
  size_t remaining = size;
  const T *it = begin;

  PREFETCH_R(it, 0);
  const bool safe = (remaining >= chunk_size);
  PREFETCH_R(it + chunk_size * safe, 0);

  uint8_t misalignment = utils::simd::misalignment_forwards(it, 64);
  misalignment -= (misalignment > remaining) * (misalignment - remaining);

  bool keep_looking = (misalignment > 0);
  while (keep_looking)
  {
    ++it;
    --misalignment;
    --remaining;

    keep_looking = comp(elem, *it);
    keep_looking &= (misalignment > 0);
  }

#if defined(__AVX512F__)
  const __m512i elem_vec = utils::simd::create_vector<__m512i, T>(elem);
  constexpr int opcode = utils::simd::get_opcode<T, Comparator>();
  __mmask64 mask = 0;

  keep_looking &= (remaining >= chunk_size);
  while (keep_looking)
  {
    const __m512i chunk = _mm512_load_si512(it);

    remaining -= chunk_size;
    it += chunk_size;
    keep_looking = (remaining >= chunk_size);

    PREFETCH_R(it + chunk_size * keep_looking, 0);

    mask = utils::simd::compare<__m512i, T>(chunk, elem_vec, opcode);
    keep_looking &= !mask;
  }
  if (mask) [[likely]]
  {
    const uint8_t matched_idx = __builtin_ctzll(mask);
    return size - (remaining + chunk_size) + matched_idx;
  }
#endif

  keep_looking &= (remaining > 0);
  while (keep_looking)
  {
    ++it;
    --remaining;

    keep_looking = comp(elem, *it);
    keep_looking &= (remaining > 0);
  }

  return remaining ? size - remaining : -1;
}

template <typename T, typename Comparator>
HOT ssize_t rfind(std::span<const T> data, const T &elem, const Comparator &comp) noexcept
{
  static_assert(std::is_integral<T>::value, "T must be an integral type");
  static_assert(std::hardware_constructive_interference_size == 64, "Cache line size must be 64 bytes");

  constexpr uint8_t chunk_size = sizeof(__m512i) / sizeof(T);

  const T *begin = data.data();
  size_t remaining = data.size();
  const T *it = begin + remaining;
  bool keep_looking = true;

  PREFETCH_R(it, 0);
  const bool safe = (remaining >= chunk_size);
  PREFETCH_R(it - chunk_size * safe, 0);

  uint8_t misalignment = utils::simd::misalignment_backwards(it, 64);
  misalignment -= (misalignment > remaining) * (misalignment - remaining);

  keep_looking = (misalignment > 0);
  while (keep_looking)
  {
    --it;
    --misalignment;
    --remaining;

    keep_looking = comp(elem, *it);
    keep_looking &= (misalignment > 0);
  }

#if defined(__AVX512F__)
  const __m512i elem_vec = utils::simd::create_vector<__m512i, T>(elem);
  constexpr int opcode = utils::simd::get_opcode<T, Comparator>();
  __mmask64 mask = 0;

  keep_looking &= (remaining >= chunk_size);
  while (keep_looking)
  {
    it -= chunk_size;
    const __m512i chunk = _mm512_load_si512(it);

    remaining -= chunk_size;
    keep_looking = (remaining >= chunk_size);

    PREFETCH_R(it - chunk_size * keep_looking, 0);

    mask = utils::simd::compare<__m512i, T>(chunk, elem_vec, opcode);
    keep_looking &= !mask;
  }

  if (mask) [[likely]]
  {
    const uint8_t matched_idx = 63 - __builtin_ctzll(mask);
    return remaining + matched_idx;
  }
#endif

  keep_looking &= (remaining > 0);
  while (keep_looking)
  {
    --it;
    --remaining;
    
    keep_looking = comp(elem, *it);
    keep_looking &= (remaining > 0);
  }

  return remaining ? remaining : -1;
}

template <typename T>
HOT ALWAYS_INLINE inline T to_host(const T &value) noexcept
{
  static_assert(std::is_arithmetic<T>::value, "T must be a numeric type");
  static_assert(sizeof(T) >= 2, "T must be at least 2 bytes");

  if constexpr (sizeof(T) == 2)
    return be16toh(value);
  if constexpr (sizeof(T) == 4)
    return be32toh(value);
  if constexpr (sizeof(T) == 8)
    return be64toh(value);

  UNREACHABLE;
}

template <typename T>
HOT ALWAYS_INLINE inline T to_network(const T &value) noexcept
{
  static_assert(std::is_arithmetic<T>::value, "T must be a numeric type");
  static_assert(sizeof(T) >= 2, "T must be at least 2 bytes");

  if constexpr (sizeof(T) == 2)
    return htobe16(value);
  if constexpr (sizeof(T) == 4)
    return htobe32(value);
  if constexpr (sizeof(T) == 8)
    return htobe64(value);

  UNREACHABLE;
}

}