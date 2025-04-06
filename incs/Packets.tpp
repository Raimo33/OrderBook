/*================================================================================

File: Packets.tpp                                                               
Creator: Claudio Raimondi                                                       
Email: claudio.raimondi@pm.me                                                   

created at: 2025-04-06 18:55:50                                                 
last edited: 2025-04-06 22:29:03                                                

================================================================================*/

#pragma once

#include "Packets.hpp"
#include "utils/utils.hpp"

template <typename T>
HOT constexpr NetworkValue<T>::NetworkValue(const T network_value) noexcept
  : value(network_value)
{
}

template <typename T>
HOT PURE ALWAYS_INLINE constexpr inline NetworkValue<T>::operator T() const noexcept
{
  return utils::to_host(value);
}

template <typename T>
HOT ALWAYS_INLINE constexpr inline NetworkValue<T> &NetworkValue<T>::operator=(const T &host_value) noexcept
{
  value = utils::to_network(host_value);
  return *this;
}

template <typename T>
PURE ALWAYS_INLINE constexpr inline T NetworkValue<T>::network_order(void) const noexcept
{
  return value;
}
