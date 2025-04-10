/*================================================================================

File: utils.cpp                                                                 
Creator: Claudio Raimondi                                                       
Email: claudio.raimondi@pm.me                                                   

created at: 2025-04-03 20:16:29                                                 
last edited: 2025-04-10 21:32:16                                                

================================================================================*/

#pragma once

#include <cstdint>
#include <span>
#include <ranges>
#include <memory>
#include <array>
#include <immintrin.h>
#include <bit>

#include "utils.hpp"
#include "macros.hpp"

namespace utils
{

//finds the first element that satisfies the comparator condition
template <typename T, typename Comparator>
HOT ssize_t find(std::span<const T> data, const T elem, const Comparator &comp) noexcept
{
  static_assert(std::is_integral<T>::value, "T must be an integral type");
  static_assert(std::hardware_constructive_interference_size == 64, "Cache line size must be 64 bytes");

  constexpr uint8_t chunk_size = sizeof(__m512i) / sizeof(T);
  const T *begin = data.data();
  const size_t size = data.size();
  size_t remaining = size;
  const T *it = begin;
  bool keep_looking;
  bool found = false;

  PREFETCH_R(it, 0);
  const bool safe = (remaining >= chunk_size);
  PREFETCH_R(it + chunk_size * safe, 0);

  uint8_t misalignment = utils::simd::misalignment_forwards(it, 64);
  misalignment = (misalignment > remaining) ? remaining : misalignment;

  keep_looking = (misalignment > 0);
  while (keep_looking)
  {
    ++it;
    --misalignment;
    --remaining;

    found = comp(*it, elem);

    keep_looking = !found;
    keep_looking &= (misalignment > 0);
  }

#ifdef __AVX512F__
  static constexpr uint8_t UNROLL_FACTOR = 8;
  static constexpr uint16_t combined_chunks_size = UNROLL_FACTOR * chunk_size;

  const __m512i elem_vec = utils::simd::create_vector<__m512i, T>(elem);
  constexpr int opcode = utils::simd::get_opcode<T, Comparator>();
  std::array<__m512i, UNROLL_FACTOR> chunks;
  std::array<__mmask64, UNROLL_FACTOR> masks;

  keep_looking &= (remaining >= combined_chunks_size);
  while (keep_looking)
  {
    it = std::assume_aligned<64>(it);

    #pragma GCC unroll UNROLL_FACTOR
    for (uint8_t i = 0; i < UNROLL_FACTOR; ++i)
      chunks[i] = _mm512_load_si512(it + i * chunk_size);

    it += combined_chunks_size;
    remaining -= combined_chunks_size;
    keep_looking = (remaining >= combined_chunks_size);

    #pragma GCC unroll UNROLL_FACTOR
    for (uint8_t i = 0; i < UNROLL_FACTOR; ++i)
      PREFETCH_R(it + i * chunk_size * keep_looking, 0);

    #pragma GCC unroll UNROLL_FACTOR
    for (uint8_t i = 0; i < UNROLL_FACTOR; ++i)
      masks[i] = utils::simd::compare<__m512i, T>(chunks[i], elem_vec, opcode);

    #pragma GCC unroll UNROLL_FACTOR
    for (uint8_t i = 0; i < UNROLL_FACTOR; ++i)
      found |= masks[i];

    keep_looking &= !found;
  }

  if (found) [[likely]]
  {
    uint8_t i = UNROLL_FACTOR;
    for (const auto &mask : masks)
    {
      i++;
      if (mask) [[likely]]
      {
        const uint8_t bit = __builtin_ctzll(mask);
        const uint16_t matched_idx = i * chunk_size + bit;
        return it - begin - combined_chunks_size + matched_idx;
      }
    }
  }
#endif

  keep_looking &= (remaining > 0);
  while (keep_looking)
  {
    ++it;
    --remaining;

    found = comp(*it, elem);

    keep_looking = !found;
    keep_looking &= (remaining > 0);
  }

  return found ? it - begin : -1;
}

//finds the first element that satisfies the comparator condition
template <typename T, typename Comparator>
HOT ssize_t rfind(std::span<const T> data, const T elem, const Comparator &comp) noexcept
{
  static_assert(std::is_integral<T>::value, "T must be an integral type");
  static_assert(std::hardware_constructive_interference_size == 64, "Cache line size must be 64 bytes");

  constexpr uint8_t chunk_size = sizeof(__m512i) / sizeof(T);
  const T *begin = data.data();
  size_t remaining = data.size();
  const T *it = begin + remaining;
  bool keep_looking;
  bool found = false;

  PREFETCH_R(it, 0);
  const bool safe = (remaining >= chunk_size);
  PREFETCH_R(it - chunk_size * safe, 0);

  uint8_t misalignment = utils::simd::misalignment_backwards(it, 64);
  misalignment = (misalignment > remaining) ? remaining : misalignment;

  keep_looking = (misalignment > 0);
  while (keep_looking)
  {
    --it;
    --misalignment;
    --remaining;

    found = comp(*it, elem);

    keep_looking = !found;
    keep_looking &= (misalignment > 0);
  }

#ifdef __AVX512F__
  static constexpr uint8_t UNROLL_FACTOR = 8;
  static constexpr uint16_t combined_chunks_size = UNROLL_FACTOR * chunk_size;

  const __m512i elem_vec = utils::simd::create_vector<__m512i, T>(elem);
  constexpr int opcode = utils::simd::get_opcode<T, Comparator>();
  std::array<__m512i, UNROLL_FACTOR> chunks;
  std::array<__mmask64, UNROLL_FACTOR> masks;

  keep_looking &= (remaining >= combined_chunks_size);
  while (keep_looking)
  {
    #pragma GCC unroll UNROLL_FACTOR
    for (uint8_t i = 0; i < UNROLL_FACTOR; ++i)
      chunks[i] = _mm512_load_si512(it - i * chunk_size);

    it -= combined_chunks_size;
    remaining -= combined_chunks_size;
    keep_looking = (remaining >= combined_chunks_size);

    #pragma GCC unroll UNROLL_FACTOR
    for (uint8_t i = 0; i < UNROLL_FACTOR; ++i)
      PREFETCH_R(it - i * chunk_size * keep_looking, 0);

    #pragma GCC unroll UNROLL_FACTOR
    for (uint8_t i = 0; i < UNROLL_FACTOR; ++i)
      masks[i] = utils::simd::compare<__m512i, T>(chunks[i], elem_vec, opcode);

    #pragma GCC unroll UNROLL_FACTOR
    for (uint8_t i = 0; i < UNROLL_FACTOR; ++i)
      found |= masks[i];

    keep_looking &= !found;
  }

  if (found) [[likely]]
  {
    uint8_t i = UNROLL_FACTOR;
    for (const auto &mask : masks | std::views::reverse)
    {
      i--;
      if (mask) [[likely]]
      {
        const uint8_t bit = 63 - __builtin_ctzll(mask);
        const uint16_t matched_idx = i * chunk_size + bit;
        return it - begin + matched_idx;
      }
    }
  }
#endif

  keep_looking &= (remaining > 0);
  while (keep_looking)
  {
    --it;
    --remaining;

    found = comp(*it, elem);

    keep_looking = !found;
    keep_looking &= (remaining > 0);
  }

  return found ? it - begin : -1;
}

constexpr bool needs_byte_swap = (std::endian::native == std::endian::little);

template <typename T>
HOT PURE ALWAYS_INLINE inline T to_host(const T &value) noexcept
{
  if constexpr (needs_byte_swap)
    return std::byteswap(value);
  else
    return value;
}

template <typename T>
HOT PURE ALWAYS_INLINE inline T to_network(const T &value) noexcept
{
  if constexpr (needs_byte_swap)
    return std::byteswap(value);
  else
    return value;
}

}