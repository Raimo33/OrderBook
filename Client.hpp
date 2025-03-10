#pragma once

#include <cstdint>
#include <string>
#include <string_view>
#include <netinet/in.h>

#include "OrderBook.hpp"
#include "TcpHandler.hpp"
#include "UdpHandler.hpp"

class Client
{
  public:
    Client(const Config& config);
    ~Client(void);

    void run(void);

  private:
    TcpHandler tcp_handler;
    UdpHandler udp_handler;
    OrderBook order_book;
};