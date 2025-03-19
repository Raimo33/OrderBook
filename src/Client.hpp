#pragma once

#include <cstdint>
#include <string>
#include <string_view>
#include <netinet/in.h>
#include <memory>
#include <chrono>

#include "Config.hpp"
#include "SanityChecker.hpp"
#include "OrderBook.hpp"
#include "MessageDispatcher.hpp"
#include "Packets.hpp"

class Client
{
  public:
    Client(void);
    ~Client(void) noexcept;

    void run(void);

  private:

    sockaddr_in createAddress(const std::string_view ip, const std::string_view port) const noexcept;
    int createTcpSocket(void) const;
    int createUdpSocket(void) const;

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
    SanityChecker sanity_checker;
    OrderBook order_book;
    MessageDispatcher dispatcher;
    const sockaddr_in glimpse_address;
    const sockaddr_in multicast_address;
    const sockaddr_in rewind_address;
    const int tcp_sock_fd;
    const int udp_sock_fd;
};