/*================================================================================

File: utils.cpp                                                                 
Creator: Claudio Raimondi                                                       
Email: claudio.raimondi@pm.me                                                   

created at: 2025-04-03 20:16:29                                                 
last edited: 2025-04-03 21:37:23                                                

================================================================================*/

#include <cstdint>
#include <vector>
#include <immintrin.h>

#include "utils.hpp"
#include "simd_utils.hpp"
#include "macros.hpp"

template <typename T, typename Comparator>
typename std::vector<T>::const_iterator utils::find(const std::vector<T> &vec, const T &elem, const Comparator &comp)
{
  static_assert(std::is_integral<T>::value, "T must be an integral type");
  static_assert(std::hardware_constructive_interference_size == 64, "Cache line size must be 64 bytes");

  constexpr uint8_t chunk_size = sizeof(__m512i) / sizeof(T);

  const T *begin = vec.data();
  size_t remaining = vec.size();
  const T *it = begin;

  PREFETCH_R(it, 0);
  const bool safe = (remaining >= chunk_size);
  PREFETCH_R(it + chunk_size * safe, 0);

  uint8_t misalignment = misalignment_forwards(it, 64);
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

  if (misalignment)
    return vec.end() - remaining;

#if defined(__AVX512F__)
  const __m512i elem_vec = create_simd_vector<__m512i, T>(elem);
  constexpr int opcode = get_simd_opcode<T, Comparator>();
  __mmask64 mask = 0;

  keep_looking = (remaining >= chunk_size);
  while (keep_looking)
  {
    const __m512i chunk = _mm512_loadu_si512(it);

    remaining -= chunk_size;
    it += chunk_size;
    keep_looking = (remaining >= chunk_size);

    PREFETCH_R(it + chunk_size * keep_looking, 0);

    mask = compare_simd<__m512i, T>(chunk, elem_vec, opcode);
    keep_looking &= !mask;
  }
  if (LIKELY(mask))
  {
    const uint8_t matched_idx = __builtin_ctzll(mask);
    return vec.end() - remaining - chunk_size + matched_idx;
  }
#endif

  keep_looking = (remaining > 0);
  while (keep_looking)
  {
    ++it;
    --remaining;

    keep_looking = comp(elem, *it);
    keep_looking &= (remaining > 0);
  }

  return vec.end() - remaining;
}

template <typename T, typename Comparator>
typename std::vector<T>::iterator utils::rfind(const std::vector<T> &vec, const T &elem, const Comparator &comp)
{
  static_assert(std::is_integral<T>::value, "T must be an integral type");
  static_assert(std::hardware_constructive_interference_size == 64, "Cache line size must be 64 bytes");

  constexpr uint8_t chunk_size = sizeof(__m512i) / sizeof(T);

  const T *begin = vec.data();
  size_t remaining = vec.size();
  const T *it = begin + remaining;
  bool keep_looking = true;

  PREFETCH_R(it, 0);
  const bool safe = (remaining >= chunk_size);
  PREFETCH_R(it - chunk_size * safe, 0);

  uint8_t misalignment = misalignment_backwards(it, 64);
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

  if (misalignment)
    return vec.begin() + remaining;

#if defined(__AVX512F__)
  const __m512i elem_vec = create_simd_vector<__m512i, T>(elem);
  constexpr int opcode = get_simd_opcode<T, Comparator>();
  __mmask64 mask = 0;

  keep_looking = (remaining >= chunk_size);
  while (keep_looking)
  {
    it -= chunk_size;
    const __m512i chunk = _mm512_loadu_si512(it);

    remaining -= chunk_size;
    keep_looking = (remaining >= chunk_size);

    PREFETCH_R(it - chunk_size * keep_looking, 0);

    mask = compare_simd<__m512i, T>(chunk, elem_vec, opcode);
    keep_looking &= !mask;
  }

  if (LIKELY(mask))
  {
    const uint8_t matched_idx = 63 - __builtin_ctzll(mask);
    return vec.begin() + remaining + matched_idx;
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

  return remaining ? vec.begin() + remaining : vec.end();
}

template <typename T>
consteval T utils::to_host(const T &value) noexcept
{
  static_assert(std::is_numeric<T>::value, "T must be a numeric type");

  if constexpr (sizeof(T) == 2)
    return be16toh(value);
  if constexpr (sizeof(T) == 4)
    return be32toh(value);
  if constexpr (sizeof(T) == 8)
    return be64toh(value);
  else
    static_assert(false, "Unsupported type size");
}

template <typename T>
consteval T utils::to_network(const T &value) noexcept
{
  static_assert(std::is_numeric<T>::value, "T must be a numeric type");

  if constexpr (sizeof(T) == 2)
    return htobe16(value);
  if constexpr (sizeof(T) == 4)
    return htobe32(value);
  if constexpr (sizeof(T) == 8)
    return htobe64(value);
  else
    static_assert(false, "Unsupported type size");
}
