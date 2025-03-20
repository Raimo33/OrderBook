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

extern bool error;

ErrorHandler::ErrorHandler(void)
{
  struct sigaction sa{};
  sa.sa_handler = [](int) { error = true; };

  error |= (sigaction(SIGINT, &sa, nullptr) == -1);
  error |= (sigaction(SIGTERM, &sa, nullptr) == -1);
  error |= (sigaction(SIGQUIT, &sa, nullptr) == -1);
  error |= (sigaction(SIGPIPE, &sa, nullptr) == -1);
}

ErrorHandler::~ErrorHandler(void) noexcept {}

void ErrorHandler::start(void)
{
  while (true)
  {
    if (UNLIKELY(error))
      throw std::runtime_error("Error occured, shit your pants");
    _mm_pause();
  }
}