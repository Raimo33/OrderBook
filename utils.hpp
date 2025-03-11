#pragma once

#include <stdexcept>
#include <string>
#include <string_view>
#include <chrono>

#include "macros.hpp"

namespace utils
{
  void throw_exception(const std::string_view message);
  void assert(const bool condition, const std::string_view message = "Assertion failed");
  sockaddr_in create_address(const std::string_view ip, const uint16_t port) noexcept;
  int create_timer(const std::chrono::milliseconds interval);
  size_t try_tcp_send(const int sock_fd, const char *buffer, const size_t size);
  size_t try_udp_send(const int sock_fd, const char *buffer, const size_t size);
  size_t try_tcp_recv(const int sock_fd, char *buffer, const size_t size);
  size_t try_udp_recv(const int sock_fd, char *buffer, const size_t size);
}