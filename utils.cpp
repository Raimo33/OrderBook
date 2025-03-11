#include "utils.hpp"
#include "macros.hpp"

#include <stdexcept>
#include <string>
#include <string_view>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <cstdint>

namespace utils
{
  NEVER_INLINE void throw_exception(const std::string_view message)
  {
    throw std::runtime_error(std::string(message));
  }

  ALWAYS_INLINE void assert(const bool condition, const std::string_view message = "Assertion failed")
  {
    if (!condition)
      throw_exception(message);
  }

  sockaddr_in init_address(const std::string_view ip, const uint16_t port)
  {
    return {
      .sin_family = AF_INET,
      .sin_port = htons(port),
      .sin_addr = {
        .s_addr = inet_addr(ip.data())
      }
    };
  }
}