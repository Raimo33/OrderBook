#pragma once

#include <string>
#include <string_view>
#include <netinet/in.h>

#include "macros.hpp"

namespace utils
{
  COLD NEVER_INLINE void throw_exception(const std::string_view message);
  HOT ALWAYS_INLINE inline void assert(const bool condition, const std::string_view message);
  COLD sockaddr_in create_address(const std::string_view ip, const uint16_t port) noexcept;
  COLD int create_timer(const std::chrono::milliseconds interval);
  HOT inline uint64_t atoul(const std::string_view str);
  HOT inline size_t try_tcp_send(const int sock_fd, const char *buffer, const size_t size);
  HOT inline size_t try_udp_send(const int sock_fd, const char *buffer, const size_t size);
  HOT inline size_t try_recv(const int sock_fd, char *buffer, const size_t size);
}

#include "utils.inl"