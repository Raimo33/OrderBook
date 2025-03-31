/*================================================================================

File: Client.hpp                                                                
Creator: Claudio Raimondi                                                       
Email: claudio.raimondi@pm.me                                                   

created at: 2025-03-23 17:58:46                                                 
last edited: 2025-03-31 20:01:59                                                

================================================================================*/

#pragma once

#include <cstdint>
#include <string>
#include <string_view>
#include <netinet/in.h>
#include <memory>
#include <chrono>

#include "OrderBook.hpp"
#include "Packets.hpp"

class Client
{
  public:
    Client(const std::string_view username, const std::string_view password) noexcept;
    ~Client(void) noexcept;

    void run(void);

  private:

    sockaddr_in createAddress(const std::string_view ip, const std::string_view port) const noexcept;
    int createTcpSocket(void) const noexcept;
    int createUdpSocket(void) const noexcept;

    void fetchOrderbook(void);
    void updateOrderbook(void);

    void sendLogin(void) const;
    void recvLogin(void);
    void recvSnapshot(void);
    void sendLogout(void) const;

    uint16_t processMessageBlocks(const std::vector<char> &buffer);
    uint16_t processMessageBlocks(const char *restrict buffer, uint16_t blocks_count);
    
    void handleSnapshotCompletion(const MessageBlock &block);
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

    const std::string username;
    const std::string password;
    OrderBook order_book;
    const sockaddr_in glimpse_address;
    const sockaddr_in multicast_address;
    const sockaddr_in rewind_address;
    const sockaddr_in bind_address_tcp;
    const sockaddr_in bind_address_udp;
    const int tcp_sock_fd;
    const int udp_sock_fd;
    uint64_t sequence_number;
};