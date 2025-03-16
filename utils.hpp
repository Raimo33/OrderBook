#pragma once

#include <string>
#include <string_view>
#include <netinet/in.h>

#include "macros.hpp"

namespace utils
{
  void throw_exception(const std::string_view message);
  inline void assert(const bool condition, const std::string_view message);
  uint64_t atoul(const std::string_view str);
  uint8_t ultoa(uint64_t num, char *buffer);
  inline size_t safe_send(const int sock_fd, const char *buffer, const size_t size);
  inline size_t safe_recv(const int sock_fd, char *buffer, const size_t size);
  uint16_t swap16(const uint16_t value);
  uint32_t swap32(const uint32_t value);
  uint64_t swap64(const uint64_t value);
}

#include "utils.inl"