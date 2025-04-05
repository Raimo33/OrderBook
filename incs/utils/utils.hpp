/*================================================================================

File: utils.hpp                                                                 
Creator: Claudio Raimondi                                                       
Email: claudio.raimondi@pm.me                                                   

created at: 2025-04-03 20:16:29                                                 
last edited: 2025-04-05 10:36:57                                                

================================================================================*/

#pragma once

#include <vector>
#include <cstdint>

#include "simd/simd_utils.hpp"

namespace utils
{
  template <typename T, typename Comparator>
  ssize_t find(const std::vector<T> &vector, const T &elem, const Comparator &comp) noexcept;

  template <typename T, typename Comparator>
  ssize_t rfind(const std::vector<T> &vector, const T &elem, const Comparator &comp) noexcept;

  template <typename T>
  T to_host(const T &value) noexcept;

  template <typename T>
  T to_network(const T &value) noexcept;
}

#include "utils.tpp"