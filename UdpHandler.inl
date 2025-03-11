#pragma once

#include "UdpHandler.hpp"

inline int UdpHandler::get_sock_fd(void) const noexcept
{
  return sock_fd;
}

inline int UdpHandler::get_timer_fd(void) const noexcept
{
  return timer_fd;
}