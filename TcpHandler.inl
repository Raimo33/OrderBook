#pragma once

#include "TcpHandler.hpp"

inline bool TcpHandler::has_finished(void) const noexcept
{
  return (state == 0);
}

inline int TcpHandler::get_sock_fd(void) const noexcept
{
  return sock_fd;
}

inline int TcpHandler::get_timer_fd(void) const noexcept
{
  return timer_fd;
}