#pragma once

#include <cstdint>
#include <string>
#include <string_view>
#include <netinet/in.h>
#include <memory>
#include <chrono>

#include "Config.hpp"
#include "OrderBook.hpp"
#include "Packets.hpp"

class Client
{
  public:
    Client(void);
    ~Client(void);

    void run(void);

  private:

    sockaddr_in create_address(const std::string_view ip, const std::string_view port) const noexcept;
    ip_mreq create_mreq(const std::string_view bind_ip_str) const noexcept;
    int create_tcp_socket(void) const noexcept;
    int create_udp_socket(void) const noexcept;

    void fetch_orderbook(void);
    void update_orderbook(void);

    bool send_login(void);
    bool recv_login(void);
    bool recv_snapshot(void);
    bool send_logout(void);

    bool process_message_blocks(const std::vector<char> &buffer);

    void handle_new_order(const MessageBlock &block);
    void handle_deleted_order(const MessageBlock &block);

    Config config;
    OrderBook order_book;
    const sockaddr_in glimpse_address;
    const sockaddr_in multicast_address;
    const sockaddr_in rewind_address;
    const int tcp_sock_fd;
    const int udp_sock_fd;
};