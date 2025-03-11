#pragma once

#include "macros.hpp"

#include <stdexcept>

namespace utils
{
  void throw_exception(const std::string_view message);
  void assert(const bool condition, const std::string_view message = "Assertion failed");
  sockaddr_in create_address(const std::string_view ip, const uint16_t port);
}