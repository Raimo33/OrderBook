#include <sys/socket.h>

#include "MessageDispatcher.hpp"

COLD MessageDispatcher::MessageDispatcher(SanityChecker &sanity_checker) :
  sanity_checker(sanity_checker) {}

COLD MessageDispatcher::~MessageDispatcher(void) {}

HOT size_t MessageDispatcher::send(const int sock_fd, const char *buffer, const size_t size) const
{
  const ssize_t ret = ::send(sock_fd, buffer, size, MSG_DONTROUTE | MSG_NOSIGNAL);

  if (UNLIKELY(ret < 0))
    SanityChecker::throwException("Failed to send data");

  sanity_checker.updateLastSent();
  return ret;
}

HOT size_t MessageDispatcher::recv(const int sock_fd, char *buffer, const size_t size) const
{
  const ssize_t ret = ::recv(sock_fd, buffer, size, MSG_NOSIGNAL);

  if (UNLIKELY(ret < 0))
    SanityChecker::throwException("Failed to receive data");

  sanity_checker.updateLastReceived();
  return ret;
}