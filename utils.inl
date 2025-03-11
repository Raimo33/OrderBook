#pragma once

#include "utils.hpp"
#include "macros.hpp"

#include <string>
#include <string_view>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <cstdint>
#include <sys/timerfd.h>

static inline size_t try_send(const int sock_fd, const char *buffer, const size_t size, const uint32_t flags);

namespace utils
{
  HOT ALWAYS_INLINE inline void assert(const bool condition, const std::string_view message = "Assertion failed")
  {
    if (UNLIKELY(!condition))
      throw_exception(message);
  }

  HOT inline size_t try_tcp_send(const int sock_fd, const char *buffer, const size_t size)
  {
    return try_send(sock_fd, buffer, size, MSG_DONTROUTE | MSG_NOSIGNAL | MSG_DONTWAIT | MSG_ZEROCOPY);
  }

  HOT inline size_t try_udp_send(const int sock_fd, const char *buffer, const size_t size)
  {
    return try_send(sock_fd, buffer, size, MSG_DONTROUTE | MSG_NOSIGNAL | MSG_DONTWAIT);
  }

  HOT inline size_t try_recv(const int sock_fd, char *buffer, const size_t size)
  {
    const ssize_t ret = recv(sock_fd, buffer, size, MSG_DONTWAIT | MSG_NOSIGNAL);

    const bool error = (ret < 0) & !((errno == EAGAIN) | (errno == EWOULDBLOCK));
    if (UNLIKELY(error))
      throw_exception("Failed to receive data");

    return ret;
  }
}

HOT static inline size_t try_send(const int sock_fd, const char *buffer, const size_t size, const uint32_t flags)
{
  const ssize_t ret = send(sock_fd, buffer, size, flags);

  const bool error = (ret < 0) & !((errno == EAGAIN) | (errno == EWOULDBLOCK));
  if (UNLIKELY(error))
    utils::throw_exception("Failed to send data");

  return ret;
}