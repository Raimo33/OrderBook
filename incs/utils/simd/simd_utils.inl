/*================================================================================

File: simd_utils.inl                                                            
Creator: Claudio Raimondi                                                       
Email: claudio.raimondi@pm.me                                                   

created at: 2025-04-04 21:21:27                                                 
last edited: 2025-04-06 22:29:03                                                

================================================================================*/

#pragma once

#include <immintrin.h>
#include "simd_utils.hpp"

HOT PURE ALWAYS_INLINE inline uint8_t utils::simd::misalignment_forwards(const void *ptr, const uint8_t alignment)
{
  return -((uintptr_t)(ptr) & (alignment - 1));
}

HOT PURE ALWAYS_INLINE inline uint8_t utils::simd::misalignment_backwards(const void *ptr, const uint8_t alignment)
{
  return (uintptr_t)(ptr) & (alignment - 1);
}