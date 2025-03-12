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

  COLD sockaddr_in create_address(const std::string_view ip, const uint16_t port) noexcept
  {
    return {
      .sin_family = AF_INET,
      .sin_port = htons(port),
      .sin_addr = {
        .s_addr = inet_addr(ip.data())
      },
      .sin_zero{}
    };
  }

  COLD int create_timer(const std::chrono::milliseconds interval)
  {
    const int timer_fd = timerfd_create(CLOCK_MONOTONIC, TFD_NONBLOCK);

    const auto seconds = std::chrono::duration_cast<std::chrono::seconds>(interval);
    const auto nanoseconds = interval - seconds;

    const struct itimerspec timer_spec = {
      .it_interval = { .tv_sec = seconds.count(), .tv_nsec = nanoseconds.count() },
      .it_value = { .tv_sec = seconds.count(), .tv_nsec = nanoseconds.count() }
    };

    timerfd_settime(timer_fd, 0, &timer_spec, nullptr);

    return timer_fd;
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