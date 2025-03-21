#include <stdexcept>

#include "error.h"
#include "macros.hpp"

HOT ALWAYS_INLINE inline void ignore(void) {}

[[noreturn]] COLD NEVER_INLINE void panic(void)
{
  throw std::runtime_error("Error occured, shit your pants");
}