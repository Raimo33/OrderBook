/*================================================================================

File: macros.h                                                                  
Creator: Claudio Raimondi                                                       
Email: claudio.raimondi@pm.me                                                   

created at: 2025-03-08 18:21:38                                                 
last edited: 2025-03-22 22:23:11                                                

================================================================================*/

#pragma once

#define LIKELY(x)                 __builtin_expect(!!(x), 1)
#define UNLIKELY(x)               __builtin_expect(!!(x), 0)
#define HOT                       __attribute__((hot))
#define COLD                      __attribute__((cold))
#define ALWAYS_INLINE             __attribute__((always_inline))
#define NEVER_INLINE              __attribute__((noinline))
#define UNREACHABLE               __builtin_unreachable()
#define UNUSED                    __attribute__((unused))
#define PREFETCH_R(x, priority)   __builtin_prefetch(x, 0, priority)
#define PREFETCH_W(x, priority)   __builtin_prefetch(x, 1, priority)
