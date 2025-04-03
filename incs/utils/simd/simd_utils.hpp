#pragma once

#include "macros.hpp"

#define misalignment_forward(ptr, alignment)    (-(uintptr_t)ptr & (alignment - 1))
#define misalignment_backwards(ptr, alignment)  ((uintptr_t)ptr & (alignment - 1))

namespace utils::simd
{
  template <typename VectorType, typename ScalarType>
  ALWAYS_INLINE inline constexpr static VectorType create_vector(const ScalarType &elem)

  template <typename ScalarType, typename Comparator>
  ALWAYS_INLINE inline static consteval int get_opcode(void)

  template <typename VectorType, typename ScalarType>
  ALWAYS_INLINE inline static __mmask64 compare(const VectorType &chunk, const VectorType &elem_vec, const int opcode)
}

#include "simd_utils.tpp"