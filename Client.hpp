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

    void Run(void);

  private:

    sockaddr_in createAddress(const std::string_view ip, const std::string_view port) const noexcept;
    ip_mreq createMreq(const std::string_view bind_ip_str) const noexcept;
    int createTcpSocket(void) const noexcept;
    int createUdpSocket(void) const noexcept;

    void fetchOrderbook(void);
    void updateOrderbook(void);

    bool sendLogin(void);
    bool recvLogin(void);
    bool recvSnapshot(void);
    bool sendLogout(void);

    bool processMessageBlocks(const std::vector<char> &buffer);

    void handleNewOrder(const MessageBlock &block);
    void handleDeletedOrder(const MessageBlock &block);

    Config config;
    OrderBook order_book;
    const sockaddr_in glimpse_address;
    const sockaddr_in multicast_address;
    const sockaddr_in rewind_address;
    const int tcp_sock_fd;
    const int udp_sock_fd;
};