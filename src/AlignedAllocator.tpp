/*================================================================================

File: AlignedAllocator.tpp                                                      
Creator: Claudio Raimondi                                                       
Email: claudio.raimondi@pm.me                                                   

created at: 2025-03-22 11:22:28                                                 
last edited: 2025-03-22 11:22:28                                                

================================================================================*/

#pragma once

#include <cstdint>

#include "AlignedAllocator.hpp"

template <typename T, std::size_t Alignment>
T *AlignedAllocator<T, Alignment>::allocate(const std::size_t size) const
{
  void *ptr = operator new(size * sizeof(T), std::align_val_t(Alignment));
  return static_cast<T *>(ptr);
}

template <typename T, std::size_t Alignment>
void AlignedAllocator<T, Alignment>::deallocate(T *ptr, const std::size_t size) const noexcept
{
  operator delete(ptr, size * sizeof(T), std::align_val_t(Alignment));
}

