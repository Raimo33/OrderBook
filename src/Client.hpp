/*================================================================================

File: Client.hpp                                                                
Creator: Claudio Raimondi                                                       
Email: claudio.raimondi@pm.me                                                   

created at: 2025-03-23 17:58:46                                                 
last edited: 2025-03-31 21:52:02                                                

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

    void processSnapshot(const std::vector<char> &buffer);
    void processMessageBlocks(const char *restrict buffer, uint16_t blocks_count);
    void processMessageData(const MessageData &data);
    
    void handleSnapshotCompletion(const MessageData &block);
    void handleNewOrder(const MessageData &block);
    void handleDeletedOrder(const MessageData &block);
    void handleSeconds(const MessageData &block);
    void handleSeriesInfoBasic(const MessageData &block);
    void handleSeriesInfoBasicCombination(const MessageData &block);
    void handleTickSizeData(const MessageData &block);
    void handleSystemEvent(const MessageData &block);
    void handleTradingStatus(const MessageData &block);
    void handleExecutionNotice(const MessageData &block);
    void handleExecutionNoticeWithTradeInfo(const MessageData &block);
    void handleEquilibriumPrice(const MessageData &block);

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
    enum Status { FETCHING, UPDATING } status;
};