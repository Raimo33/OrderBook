#pragma once

#include <cstdint>
#include <string>
#include <string_view>
#include <netinet/in.h>
#include <memory>

#include "OrderBook.hpp"
#include "TcpHandler.hpp"
#include "UdpHandler.hpp"
#include "Config.hpp"

class Client
{
  public:
    Client(const Config &config);
    ~Client(void);

    void run(void);

  private:
    void create_handlers(void);
    void add_to_epoll(const int fd);
    void remove_from_epoll(const int fd);
    void switch_server(void);

    Config config;
    uint8_t server_idx;
    std::unique_ptr<TcpHandler> tcp_handler;
    std::unique_ptr<UdpHandler> udp_handler;
    OrderBook order_book;
    int epoll_fd;
};