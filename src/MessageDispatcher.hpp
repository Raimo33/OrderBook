#pragma once

#include <cstdint>

#include "SanityChecker.hpp"

class MessageDispatcher
{
  public:
    MessageDispatcher(SanityChecker &sanity_checker);
    ~MessageDispatcher(void);

    //TODO maybe use string_view
    size_t send(const int sock_fd, const char *buffer, const size_t size) const;
    size_t recv(const int sock_fd, char *buffer, const size_t size) const;

  private:
    SanityChecker &sanity_checker;
};