/*================================================================================

File: utils.cpp                                                                 
Creator: Claudio Raimondi                                                       
Email: claudio.raimondi@pm.me                                                   

created at: 2025-04-03 20:16:29                                                 
last edited: 2025-04-24 13:13:44                                                

================================================================================*/

#pragma once

#include <cstdint>
#include <span>
#include <ranges>
#include <array>
#include <immintrin.h>

#include "utils.hpp"
#include "macros.hpp"

namespace utils
{

template <typename T, typename Comparator>
HOT ssize_t forward_lower_bound(std::span<const T> data, const T elem, const Comparator &comp) noexcept
{
  static_assert(std::is_integral<T>::value, "T must be an integral type");
  static_assert(std::hardware_constructive_interference_size == 64, "Cache line size must be 64 bytes");

  static constexpr uint8_t chunk_size = sizeof(__m512i) / sizeof(T);
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
    found = comp(*it, elem);

    --misalignment;
    --remaining;

    keep_looking = (misalignment > 0);
    keep_looking &= !found;

    it += keep_looking;
  }

#ifdef __AVX512F__
  static constexpr uint8_t UNROLL_FACTOR = 8;
  static constexpr uint16_t combined_chunks_size = UNROLL_FACTOR * chunk_size;

  const __m512i elem_vec = utils::simd::create_vector<__m512i, T>(elem);
  constexpr int opcode = utils::simd::get_opcode<T, Comparator>();
  std::array<__m512i, UNROLL_FACTOR> chunks;
  std::array<__mmask64, UNROLL_FACTOR> masks;

  keep_looking = (remaining >= combined_chunks_size);
  keep_looking &= !found;

  while (keep_looking)
  {
    it = std::assume_aligned<64>(it);

    #pragma GCC unroll UNROLL_FACTOR
    for (uint8_t i = 0; i < UNROLL_FACTOR; ++i)
      chunks[i] = _mm512_load_si512(it + i * chunk_size);

    it += combined_chunks_size;
    remaining -= combined_chunks_size;
    keep_looking = (remaining >= combined_chunks_size);

    const uint8_t safe_chunk_size = chunk_size * keep_looking; 
    #pragma GCC unroll UNROLL_FACTOR
    for (uint8_t i = 0; i < UNROLL_FACTOR; ++i)
      PREFETCH_R(it + i * safe_chunk_size, 0);

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

  keep_looking = (remaining > 0);
  keep_looking &= !found;

  while (keep_looking)
  {
    found = comp(*it, elem);

    --remaining;

    keep_looking = (remaining > 0);
    keep_looking &= !found;

    it += keep_looking;
  }

  return found ? it - begin : -1;
}

template <typename T, typename Comparator>
HOT ssize_t backward_lower_bound(std::span<const T> data, const T elem, const Comparator &comp) noexcept
{
  static_assert(std::is_integral<T>::value, "T must be an integral type");
  static_assert(std::hardware_constructive_interference_size == 64, "Cache line size must be 64 bytes");

  static constexpr uint8_t chunk_size = sizeof(__m512i) / sizeof(T);
  const T *begin = data.data();
  size_t remaining = data.size();
  const T *it = begin + remaining - 1;
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
    found = comp(*it, elem);

    --misalignment;
    --remaining;

    keep_looking = (misalignment > 0);
    keep_looking &= !found;

    it -= keep_looking;
  }

#ifdef __AVX512F__
  static constexpr uint8_t UNROLL_FACTOR = 8;
  static constexpr uint16_t combined_chunks_size = UNROLL_FACTOR * chunk_size;

  const __m512i elem_vec = utils::simd::create_vector<__m512i, T>(elem);
  constexpr int opcode = utils::simd::get_opcode<T, Comparator>();
  std::array<__m512i, UNROLL_FACTOR> chunks;
  std::array<__mmask64, UNROLL_FACTOR> masks;

  keep_looking = (remaining >= combined_chunks_size);
  keep_looking &= !found;

  while (keep_looking)
  {
    #pragma GCC unroll UNROLL_FACTOR
    for (uint8_t i = 0; i < UNROLL_FACTOR; ++i)
      chunks[i] = _mm512_load_si512(it - i * chunk_size);

    it -= combined_chunks_size;
    remaining -= combined_chunks_size;
    keep_looking = (remaining >= combined_chunks_size);

    const uint8_t safe_chunk_size = chunk_size * keep_looking;
    #pragma GCC unroll UNROLL_FACTOR
    for (uint8_t i = 0; i < UNROLL_FACTOR; ++i)
      PREFETCH_R(it - i * safe_chunk_size, 0);

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

  keep_looking = (remaining > 0);
  keep_looking &= !found;

  while (keep_looking)
  {
    found = comp(*it, elem);

    --remaining;

    keep_looking = (remaining > 0);
    keep_looking &= !found;

    it -= keep_looking;
  }

  return found ? it - begin : -1;
}

}