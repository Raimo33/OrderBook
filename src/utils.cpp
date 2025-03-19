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
  
  HOT uint8_t ultoa(uint64_t num, char *buffer)
  {
    if (num == 0)
    {
      buffer[0] = '0';
      return 1;
    }
  
    const uint8_t digits = 1 +
      (num >= 10UL) +
      (num >= 100UL) +
      (num >= 1000UL) +
      (num >= 10000UL) +
      (num >= 100000UL) +
      (num >= 1000000UL) +
      (num >= 10000000UL) +
      (num >= 100000000UL) +
      (num >= 1000000000UL) +
      (num >= 10000000000UL) +
      (num >= 100000000000UL) +
      (num >= 1000000000000UL) +
      (num >= 10000000000000UL) +
      (num >= 100000000000000UL) +
      (num >= 1000000000000000UL) +
      (num >= 10000000000000000UL) +
      (num >= 100000000000000000UL) +
      (num >= 1000000000000000000UL);
  
    constexpr uint64_t power10[] = {
      1UL,
      10UL,
      100UL,
      1000UL,
      10000UL,
      100000UL,
      1000000UL,
      10000000UL,
      100000000UL,
      1000000000UL,
      10000000000UL,
      100000000000UL,
      1000000000000UL,
      10000000000000UL,
      100000000000000UL,
      1000000000000000UL,
      10000000000000000UL,
      100000000000000000UL,
      1000000000000000000UL,
    };
  
    uint64_t power = power10[digits - 1];
    char *p = buffer;
    uint64_t quotient;
    for (int i = 0; i < digits; i++)
    {
      quotient = num / power;
      *p++ = (char)('0' + quotient);
      num -= quotient * power;
      power /= 10;
    }
  
    return digits;
  }
}