/*================================================================================

File: utils.hpp                                                                 
Creator: Claudio Raimondi                                                       
Email: claudio.raimondi@pm.me                                                   

created at: 2025-04-03 20:16:29                                                 
last edited: 2025-04-04 21:21:27                                                

================================================================================*/

#pragma once

#include <vector>

#include "simd/simd_utils.hpp"

namespace utils
{
  template <typename T, typename Comparator>
  typename std::vector<T>::const_iterator find(const std::vector<T> &vector, const T &elem, const Comparator &comp) noexcept;

  template <typename T, typename Comparator>
  typename std::vector<T>::iterator rfind(const std::vector<T> &vector, const T &elem, const Comparator &comp) noexcept;

  template <typename T>
  T to_host(const T &value) noexcept;

  template <typename T>
  T to_network(const T &value) noexcept;
}

#include "utils.tpp"