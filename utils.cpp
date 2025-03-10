#include "utils.hpp"
#include "macros.hpp"

#include <stdexcept>

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
}