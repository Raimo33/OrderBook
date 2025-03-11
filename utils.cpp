#include "utils.hpp"
#include "macros.hpp"

#include <stdexcept>
#include <string>
#include <string_view>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <cstdint>
#include <sys/timerfd.h>

static inline size_t try_send(const int sock_fd, const char *buffer, const size_t size, const uint32_t flags);

namespace utils
{
  COLD NEVER_INLINE void throw_exception(const std::string_view message)
  {
    throw std::runtime_error(std::string(message));
  }

  HOT ALWAYS_INLINE void assert(const bool condition, const std::string_view message = "Assertion failed")
  {
    if (!condition)
      throw_exception(message);
  }

  COLD sockaddr_in create_address(const std::string_view ip, const uint16_t port) noexcept
  {
    return {
      .sin_family = AF_INET,
      .sin_port = htons(port),
      .sin_addr = {
        .s_addr = inet_addr(ip.data())
      }
    };
  }

  COLD int create_timer(const std::chrono::duration<uint64_t> interval)
  {
    const int timer_fd = timerfd_create(CLOCK_MONOTONIC, TFD_NONBLOCK);

    const auto seconds = std::chrono::duration_cast<std::chrono::seconds>(interval);
    const auto nanoseconds = std::chrono::duration_cast<std::chrono::nanoseconds>(interval - seconds);

    const struct itimerspec timer_spec = {
      .it_interval = { .tv_sec = seconds.count(), .tv_nsec = nanoseconds.count() },
      .it_value = { .tv_sec = seconds.count(), .tv_nsec = nanoseconds.count() }
    };

    timerfd_settime(timer_fd, 0, &timer_spec, nullptr);

    return timer_fd;
  }

  HOT ALWAYS_INLINE inline size_t try_tcp_send(const int sock_fd, const char *buffer, const size_t size)
  {
    return try_send(sock_fd, buffer, size, MSG_DONTROUTE | MSG_NOSIGNAL | MSG_DONTWAIT | MSG_ZEROCOPY);
  }

  HOT ALWAYS_INLINE inline size_t try_udp_send(const int sock_fd, const char *buffer, const size_t size)
  {
    return try_send(sock_fd, buffer, size, MSG_DONTROUTE | MSG_NOSIGNAL | MSG_DONTWAIT);
  }

}

HOT static inline size_t try_send(const int sock_fd, const char *buffer, const size_t size, const uint32_t flags)
{
  const ssize_t ret = send(sock_fd, buffer, size, flags);

  const bool error = (ret < 0) & !((errno == EAGAIN) | (errno == EWOULDBLOCK));
  if (UNLIKELY(error))
    utils::throw_exception("Failed to send data");

  return ret;
}