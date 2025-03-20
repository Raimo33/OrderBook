#include <chrono>
#include <csignal>
#include <stdexcept>
#include <unordered_map>
#include <unistd.h>
#include <sys/timerfd.h>
#include <immintrin.h>

#include "ErrorHandler.hpp"
#include "macros.hpp"

using namespace std::chrono_literals;

extern volatile bool error;

COLD ErrorHandler::ErrorHandler(void)
{
  struct sigaction sa{};
  sa.sa_handler = [](int) { error = true; };

  error |= (sigaction(SIGINT, &sa, nullptr) == -1);
  error |= (sigaction(SIGTERM, &sa, nullptr) == -1);
  error |= (sigaction(SIGQUIT, &sa, nullptr) == -1);
  error |= (sigaction(SIGPIPE, &sa, nullptr) == -1);
}

COLD ErrorHandler::~ErrorHandler(void) noexcept {}

COLD void ErrorHandler::start(void)
{
  while (LIKELY(!error))
    _mm_pause();

  throw std::runtime_error("Error detected, shit your pants");
}