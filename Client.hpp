#pragma once

#include <cstdint>
#include <string>
#include <string_view>
#include <netinet/in.h>

#include "OrderBook.hpp"
#include "TcpHandler.hpp"
#include "UdpHandler.hpp"

struct Config;

class Client
{
  public:
    Client(const Config &config);
    ~Client(void);

    void run(void);

  private:
    void init_epoll(void);
    void switch_server(void);

    Config config;
    uint8_t server_idx;
    TcpHandler tcp_handler;
    UdpHandler udp_handler;
    OrderBook order_book;
    int epoll_fd;
};