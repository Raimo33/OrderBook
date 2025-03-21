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
    void processMessageBlocks(const char *buffer, uint16_t blocks_count);

    void handleNewOrder(const MessageBlock &block);
    void handleDeletedOrder(const MessageBlock &block);
    void handleSeconds(const MessageBlock &block);
    void handleSeriesInfoBasic(const MessageBlock &block);
    void handleSeriesInfoBasicCombination(const MessageBlock &block);
    void handleTickSizeData(const MessageBlock &block);
    void handleSystemEvent(const MessageBlock &block);
    void handleTradingStatus(const MessageBlock &block);
    void handleExecutionNotice(const MessageBlock &block);
    void handleExecutionNoticeWithTradeInfo(const MessageBlock &block);
    void handleEquilibriumPrice(const MessageBlock &block);

    Config config;
    OrderBook order_book;
    const sockaddr_in glimpse_address;
    const sockaddr_in multicast_address;
    const sockaddr_in rewind_address;
    const int tcp_sock_fd;
    const int udp_sock_fd;
    uint64_t sequence_number;
};