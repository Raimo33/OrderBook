/*================================================================================

File: error.cpp                                                                 
Creator: Claudio Raimondi                                                       
Email: claudio.raimondi@pm.me                                                   

created at: 2025-03-23 17:58:46                                                 
last edited: 2025-03-23 17:58:46                                                

================================================================================*/

#include <stdexcept>

#include "error.hpp"

[[noreturn]] COLD NEVER_INLINE void panic(void)
{
  #ifdef __EXCEPTIONS
    throw std::runtime_error("Error occured, shit your pants");
  #else
    std::terminate();
  #endif
}