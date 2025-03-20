#pragma once

#include <string_view>
#include <chrono>

#include "Packets.hpp"

//TODO disaster recovery
//TODO issue rewind on packet loss
//TODO issue glimpse on big packet loss
class ErrorHandler
{
  public:
    ErrorHandler(void);
    ~ErrorHandler(void) noexcept;

    void start(void);

  private:
    void handleSignal(void);
};