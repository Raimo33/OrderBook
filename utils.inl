#pragma once

#include "utils.hpp"
#include "macros.hpp"

#include <cstring>
#include <stdexcept>
#include <string>
#include <string_view>
#include <sys/socket.h>
#include <unistd.h>
#include <arpa/inet.h>

namespace utils
{
  HOT ALWAYS_INLINE inline void assert(const bool condition, const std::string_view message = "Assertion failed")
  {
    if (UNLIKELY(!condition))
      throw_exception(message);
  }

  HOT inline size_t safe_send(const int sock_fd, const char *buffer, const size_t size)
  {
    const ssize_t ret = send(sock_fd, buffer, size, MSG_DONTROUTE | MSG_NOSIGNAL);
    if (UNLIKELY(ret < 0))
      throw_exception("Failed to send data");
    return ret;
  }

  HOT inline size_t safe_recv(const int sock_fd, char *buffer, const size_t size)
  {
    const ssize_t ret = recv(sock_fd, buffer, size, MSG_NOSIGNAL);
    if (UNLIKELY(ret < 0))
      throw_exception("Failed to receive data");
    return ret;
  }

  HOT inline size_t safe_read(const int fd, char *buffer, const size_t size)
  {
    const ssize_t ret = read(fd, buffer, size);
    if (UNLIKELY(ret < 0))
      throw_exception("Failed to read data");
    return ret;
  }

  HOT ALWAYS_INLINE inline uint16_t bswap16(const uint16_t value)
  {
    return __builtin_bswap16(value);
  }

  HOT ALWAYS_INLINE inline uint32_t bswap32(const uint32_t value)
  {
    return __builtin_bswap32(value);
  }

  HOT ALWAYS_INLINE inline uint64_t bswap64(const uint64_t value)
  {
    return __builtin_bswap64(value);
  }
}