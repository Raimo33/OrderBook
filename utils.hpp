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
  inline size_t safe_read(const int fd, char *buffer, const size_t size);
  inline uint16_t bswap16(const uint16_t value);
  inline uint32_t bswap32(const uint32_t value);
  inline uint64_t bswap64(const uint64_t value);
}

#include "utils.inl"