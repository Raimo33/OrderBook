/*================================================================================

File: utils.hpp                                                                 
Creator: Claudio Raimondi                                                       
Email: claudio.raimondi@pm.me                                                   

created at: 2025-04-03 20:16:29                                                 
last edited: 2025-04-03 21:37:23                                                

================================================================================*/

#pragma once

#include <vector>

namespace utils
{
  template <typename T, typename Comparator>
  typename std::vector<T>::const_iterator find(const std::vector<T> &vector, const T &elem, const Comparator &comp) noexcept;

  template <typename T, typename Comparator>
  typename std::vector<T>::iterator rfind(const std::vector<T> &vector, const T &elem, const Comparator &comp) noexcept;

  template <typename T>
  consteval T to_host(const T &value) noexcept;

  template <typename T>
  consteval T to_network(const T &value) noexcept;
}