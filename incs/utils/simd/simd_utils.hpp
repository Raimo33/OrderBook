/*================================================================================

File: simd_utils.hpp                                                            
Creator: Claudio Raimondi                                                       
Email: claudio.raimondi@pm.me                                                   

created at: 2025-04-04 21:21:27                                                 
last edited: 2025-04-04 21:21:27                                                

================================================================================*/

#pragma once

#include <immintrin.h>

#include "macros.hpp"

namespace utils::simd
{
  template <typename VectorType, typename ScalarType>
  ALWAYS_INLINE inline consteval VectorType create_vector(const ScalarType &elem);

  template <typename ScalarType, typename Comparator>
  ALWAYS_INLINE inline consteval int get_opcode(void);

  template <typename VectorType, typename ScalarType>
  ALWAYS_INLINE inline __mmask64 compare(const VectorType &chunk, const VectorType &elem_vec, const int opcode);

  ALWAYS_INLINE inline uint8_t misalignment_forwards(const void *ptr, const uint8_t alignment);

  ALWAYS_INLINE inline uint8_t misalignment_backwards(const void *ptr, const uint8_t alignment);
}

#include "simd_utils.tpp"
#include "simd_utils.inl"