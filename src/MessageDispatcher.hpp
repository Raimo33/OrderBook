#pragma once

#include <cstdint>

#include "SanityChecker.hpp"

class MessageDispatcher
{
  public:
    MessageDispatcher(SanityChecker &sanity_checker);
    ~MessageDispatcher(void);

    size_t send(const int sock_fd, const char *buffer, const size_t size) const;
    size_t recv(const int sock_fd, char *buffer, const size_t size) const;
    size_t recvmmsg(const int sock_fd, struct mmsghdr *msgvec, const size_t vlen) const;  //TODO non blocking to avoid high latency

  private:
    SanityChecker &sanity_checker;
};