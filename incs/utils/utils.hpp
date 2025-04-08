/*================================================================================

File: utils.hpp                                                                 
Creator: Claudio Raimondi                                                       
Email: claudio.raimondi@pm.me                                                   

created at: 2025-04-03 20:16:29                                                 
last edited: 2025-04-08 18:40:40                                                

================================================================================*/

#pragma once

#include <span>
#include <cstdint>

#include "simd/simd_utils.hpp"

namespace utils
{
  template <typename T, typename Comparator>
  ssize_t find(std::span<T> data, const T elem, const Comparator &comp) noexcept;

  template <typename T, typename Comparator>
  ssize_t rfind(std::span<T> data, const T elem, const Comparator &comp) noexcept;

  template <typename T>
  inline T to_host(const T &value) noexcept;

  template <typename T>
  inline T to_network(const T &value) noexcept;
}

#include "utils.tpp"