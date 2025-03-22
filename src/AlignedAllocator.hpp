#pragma once

#include <cstdint>

template <typename T, std::size_t Alignment = 64>
class AlignedAllocator
{
  using value_type = T;

  template <typename U>
  struct rebind
  {
    using other = std::allocator<U>;
  };

  T *allocate(const std::size_t size) const;
  void deallocate(T *ptr, const std::size_t size) const noexcept;
};

#include "AlignedAllocator.tpp"