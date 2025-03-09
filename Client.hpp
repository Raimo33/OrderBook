#pragma once

#include <cstdint>
#include <string_view>
#include <netinet/in.h>

#include "OrderBook.hpp"
#include "TcpHandler.hpp"
#include "UdpHandler.hpp"

class Client
{
  public:
    Client(const std::string_view ip_str, const uint16_t port);
    ~Client(void);

    void run(void);

  private:
    struct sockaddr_in addr;
    TcpHandler tcp_handler;
    UdpHandler udp_handler;
    OrderBook order_book;
};