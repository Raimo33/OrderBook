/*================================================================================

File: simd_utils.tpp                                                            
Creator: Claudio Raimondi                                                       
Email: claudio.raimondi@pm.me                                                   

created at: 2025-04-04 21:21:27                                                 
last edited: 2025-04-06 17:53:26                                                

================================================================================*/

//TODO add compile time errors

#pragma once

#include <immintrin.h>
#include <cstdint>
#include <type_traits>

#include "simd_utils.hpp"
#include "macros.hpp"

template <typename VectorType, typename ScalarType>
ALWAYS_INLINE inline VectorType utils::simd::create_vector(const ScalarType &elem)
{
#ifdef __AVX512F__
  if constexpr (std::is_same_v<VectorType, __m512i>)
  {
    if constexpr (sizeof(ScalarType) == sizeof(int8_t))
      return _mm512_set1_epi8(elem);
    if constexpr (sizeof(ScalarType) == sizeof(int16_t))
      return _mm512_set1_epi16(elem);
    if constexpr (sizeof(ScalarType) == sizeof(int32_t))
      return _mm512_set1_epi32(elem);
    if constexpr (sizeof(ScalarType) == sizeof(int64_t))
      return _mm512_set1_epi64(elem);
  }
  if constexpr (std::is_same_v<VectorType, __m512>)
  {
    if constexpr (sizeof(ScalarType) == sizeof(float))
      return _mm512_set1_ps(elem);
  }
  if constexpr (std::is_same_v<VectorType, __m512d>)
  {
    if constexpr (sizeof(ScalarType) == sizeof(double))
      return _mm512_set1_pd(elem);
  }
#endif

#ifdef __AVX__
  if constexpr (std::is_same_v<VectorType, __m256i>)
  {
    if constexpr (sizeof(ScalarType) == sizeof(int8_t))
      return _mm256_set1_epi8(elem);
    if constexpr (sizeof(ScalarType) == sizeof(int16_t))
      return _mm256_set1_epi16(elem);
    if constexpr (sizeof(ScalarType) == sizeof(int32_t))
      return _mm256_set1_epi32(elem);
    if constexpr (sizeof(ScalarType) == sizeof(int64_t))
      return _mm256_set1_epi64x(elem);
  }
  if constexpr (std::is_same_v<VectorType, __m256>)
  {
    if constexpr (sizeof(ScalarType) == sizeof(float))
      return _mm256_set1_ps(elem);
  }
  if constexpr (std::is_same_v<VectorType, __m256d>)
  {
    if constexpr (sizeof(ScalarType) == sizeof(double))
      return _mm256_set1_pd(elem);
  }
#endif

#ifdef __SSE2__
  if constexpr (std::is_same_v<VectorType, __m128i>)
  {
    if constexpr (sizeof(ScalarType) == sizeof(int8_t))
      return _mm_set1_epi8(elem);
    if constexpr (sizeof(ScalarType) == sizeof(int16_t))
      return _mm_set1_epi16(elem);
    if constexpr (sizeof(ScalarType) == sizeof(int32_t))
      return _mm_set1_epi32(elem);
    if constexpr (sizeof(ScalarType) == sizeof(int64_t))
      return _mm_set1_epi64x(elem);
  }
  if constexpr (std::is_same_v<VectorType, __m128>)
  {
    if constexpr (sizeof(ScalarType) == sizeof(float))
      return _mm_set1_ps(elem);
  }
  if constexpr (std::is_same_v<VectorType, __m128d>)
  {
    if constexpr (sizeof(ScalarType) == sizeof(double))
      return _mm_set1_pd(elem);
  }
#endif
}

template <typename ScalarType, typename Comparator>
ALWAYS_INLINE inline constexpr int utils::simd::get_opcode(void)
{
  if constexpr (std::is_integral_v<ScalarType>)
  {
    if constexpr (std::is_same_v<Comparator, std::equal_to<ScalarType>>)
      return _MM_CMPINT_EQ;
    if constexpr (std::is_same_v<Comparator, std::not_equal_to<ScalarType>>)
      return _MM_CMPINT_NE;
    if constexpr (std::is_same_v<Comparator, std::less_equal<ScalarType>>)
      return _MM_CMPINT_LE;
    if constexpr (std::is_same_v<Comparator, std::greater_equal<ScalarType>>)
      return _MM_CMPINT_GE;
    if constexpr (std::is_same_v<Comparator, std::less<ScalarType>>)
      return _MM_CMPINT_LT;
    if constexpr (std::is_same_v<Comparator, std::greater<ScalarType>>)
      return _MM_CMPINT_GT;
  }
  if constexpr (std::is_floating_point_v<ScalarType>)
  {
    if constexpr (std::is_same_v<Comparator, std::equal_to<ScalarType>>)
      return _CMP_EQ_OQ;
    if constexpr (std::is_same_v<Comparator, std::not_equal_to<ScalarType>>)
      return _CMP_NEQ_OQ;
    if constexpr (std::is_same_v<Comparator, std::less_equal<ScalarType>>)
      return _CMP_LE_OQ;
    if constexpr (std::is_same_v<Comparator, std::greater_equal<ScalarType>>)
      return _CMP_GE_OQ;
    if constexpr (std::is_same_v<Comparator, std::less<ScalarType>>)
      return _CMP_LT_OQ;
    if constexpr (std::is_same_v<Comparator, std::greater<ScalarType>>)
      return _CMP_GT_OQ;
  }
}

template <typename VectorType, typename ScalarType>
ALWAYS_INLINE inline __mmask64 utils::simd::compare(const VectorType &chunk, const VectorType &elem_vec, const int opcode)
{
#ifdef __AVX512F__
  if constexpr (std::is_same<VectorType, __m512i>::value)
  {
    if constexpr (std::is_integral_v<ScalarType>)
    {
      if constexpr (std::is_signed_v<ScalarType>)
      {
        if constexpr (sizeof(ScalarType) == sizeof(int8_t))
          return _mm512_cmp_epi8_mask(chunk, elem_vec, opcode);
        if constexpr (sizeof(ScalarType) == sizeof(int16_t))
          return _mm512_cmp_epi16_mask(chunk, elem_vec, opcode);
        if constexpr (sizeof(ScalarType) == sizeof(int32_t))
          return _mm512_cmp_epi32_mask(chunk, elem_vec, opcode);
        if constexpr (sizeof(ScalarType) == sizeof(int64_t))
          return _mm512_cmp_epi64_mask(chunk, elem_vec, opcode);
      }
      else
      {
        if constexpr (sizeof(ScalarType) == sizeof(uint8_t))
          return _mm512_cmp_epu8_mask(chunk, elem_vec, opcode);
        if constexpr (sizeof(ScalarType) == sizeof(uint16_t))
          return _mm512_cmp_epu16_mask(chunk, elem_vec, opcode);
        if constexpr (sizeof(ScalarType) == sizeof(uint32_t))
          return _mm512_cmp_epu32_mask(chunk, elem_vec, opcode);
        if constexpr (sizeof(ScalarType) == sizeof(uint64_t))
          return _mm512_cmp_epu64_mask(chunk, elem_vec, opcode);
      }
    }
    if constexpr (std::is_floating_point_v<ScalarType>)
    {
      if constexpr (sizeof(ScalarType) == sizeof(float))
        return _mm512_cmp_ps_mask(chunk, elem_vec, opcode);
      if constexpr (sizeof(ScalarType) == sizeof(double))
        return _mm512_cmp_pd_mask(chunk, elem_vec, opcode);
    }
  }
#endif

#ifdef __AVX__
  if constexpr (std::is_same<VectorType, __m256i>::value)
  {
    if constexpr (std::is_integral_v<ScalarType>)
    {
      if constexpr (std::is_signed_v<ScalarType>)
      {
        if constexpr (sizeof(ScalarType) == sizeof(int8_t))
          return _mm256_cmp_epi8_mask(chunk, elem_vec, opcode);
        if constexpr (sizeof(ScalarType) == sizeof(int16_t))
          return _mm256_cmp_epi16_mask(chunk, elem_vec, opcode);
        if constexpr (sizeof(ScalarType) == sizeof(int32_t))
          return _mm256_cmp_epi32_mask(chunk, elem_vec, opcode);
        if constexpr (sizeof(ScalarType) == sizeof(int64_t))
          return _mm256_cmp_epi64_mask(chunk, elem_vec, opcode);
      }
      else
      {
        if constexpr (sizeof(ScalarType) == sizeof(uint8_t))
          return _mm256_cmp_epu8_mask(chunk, elem_vec, opcode);
        if constexpr (sizeof(ScalarType) == sizeof(uint16_t))
          return _mm256_cmp_epu16_mask(chunk, elem_vec, opcode);
        if constexpr (sizeof(ScalarType) == sizeof(uint32_t))
          return _mm256_cmp_epu32_mask(chunk, elem_vec, opcode);
        if constexpr (sizeof(ScalarType) == sizeof(uint64_t))
          return _mm256_cmp_epu64_mask(chunk, elem_vec, opcode);
      }
    }
    if constexpr (std::is_floating_point_v<ScalarType>)
    {
      if constexpr (sizeof(ScalarType) == sizeof(float))
        return _mm256_cmp_ps_mask(chunk, elem_vec, opcode);
      if constexpr (sizeof(ScalarType) == sizeof(double))
        return _mm256_cmp_pd_mask(chunk, elem_vec, opcode);
    }
  }
#endif

#ifdef __SSE2__
  if constexpr (std::is_same<VectorType, __m128i>::value)
  {
    if constexpr (std::is_integral_v<ScalarType>)
    {
      if constexpr (std::is_signed_v<ScalarType>)
      {
        if constexpr (sizeof(ScalarType) == sizeof(int8_t))
          return _mm_cmp_epi8_mask(chunk, elem_vec, opcode);
        if constexpr (sizeof(ScalarType) == sizeof(int16_t))
          return _mm_cmp_epi16_mask(chunk, elem_vec, opcode);
        if constexpr (sizeof(ScalarType) == sizeof(int32_t))
          return _mm_cmp_epi32_mask(chunk, elem_vec, opcode);
        if constexpr (sizeof(ScalarType) == sizeof(int64_t))
          return _mm_cmp_epi64_mask(chunk, elem_vec, opcode);
      }
      else
      {
        if constexpr (sizeof(ScalarType) == sizeof(uint8_t))
          return _mm_cmp_epu8_mask(chunk, elem_vec, opcode);
        if constexpr (sizeof(ScalarType) == sizeof(uint16_t))
          return _mm_cmp_epu16_mask(chunk, elem_vec, opcode);
        if constexpr (sizeof(ScalarType) == sizeof(uint32_t))
          return _mm_cmp_epu32_mask(chunk, elem_vec, opcode);
        if constexpr (sizeof(ScalarType) == sizeof(uint64_t))
          return _mm_cmp_epu64_mask(chunk, elem_vec, opcode);
      }
    }
    if constexpr (std::is_floating_point_v<ScalarType>)
    {
      if constexpr (sizeof(ScalarType) == sizeof(float))
        return _mm_cmp_ps_mask(chunk, elem_vec, opcode);
      if constexpr (sizeof(ScalarType) == sizeof(double))
        return _mm_cmp_pd_mask(chunk, elem_vec, opcode);
    }
  }
#endif
}