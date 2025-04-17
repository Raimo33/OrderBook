/*================================================================================

File: utils.hpp                                                                 
Creator: Claudio Raimondi                                                       
Email: claudio.raimondi@pm.me                                                   

created at: 2025-04-03 20:16:29                                                 
last edited: 2025-04-17 16:57:31                                                

================================================================================*/

#pragma once

#include <span>
#include <cstdint>

#include "simd/simd_utils.hpp"

namespace utils
{
  template <typename T, typename Comparator>
  ssize_t forward_lower_bound(std::span<const T> data, const T elem, const Comparator &comp) noexcept;

  template <typename T, typename Comparator>
  ssize_t backward_lower_bound(std::span<const T> data, const T elem, const Comparator &comp) noexcept;
}

#include "utils.tpp"