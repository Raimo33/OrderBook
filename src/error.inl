/*================================================================================

File: error.inl                                                                 
Creator: Claudio Raimondi                                                       
Email: claudio.raimondi@pm.me                                                   

created at: 2025-03-22 11:22:28                                                 
last edited: 2025-03-22 11:22:28                                                

================================================================================*/

#pragma once

#include <stdexcept>

#include "error.hpp"
#include "macros.hpp"

HOT ALWAYS_INLINE inline void ignore(void) {}

[[noreturn]] COLD NEVER_INLINE void panic(void)
{
  throw std::runtime_error("Error occured, shit your pants");
}