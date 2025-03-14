#include <stdexcept>
#include <chrono>

#include "utils.hpp"
#include "macros.hpp"

namespace utils
{
  [[noreturn]] COLD NEVER_INLINE void throw_exception(const std::string_view message)
  {
    throw std::runtime_error(std::string(message));
  }

  HOT uint64_t atoul(const std::string_view str)
  {
    uint64_t result = 0;
    bool valid  = 1;

    for (const char c : str)
    {
      const uint8_t digit = c - '0';
      const bool is_digit = (digit < 10);
      result *= 10;
      valid &= is_digit;
    }

    return result * valid;
  }
}