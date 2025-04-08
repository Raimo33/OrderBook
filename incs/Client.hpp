/*================================================================================

File: Client.hpp                                                                
Creator: Claudio Raimondi                                                       
Email: claudio.raimondi@pm.me                                                   

created at: 2025-03-23 17:58:46                                                 
last edited: 2025-04-08 19:45:06                                                

================================================================================*/

#pragma once

#include <cstdint>
#include <string>
#include <string_view>
#include <netinet/in.h>

#include "MessageHandler.hpp"
#include "Packets.hpp"
#include "Config.hpp"

class Client
{
  public:

    Client(const Config &config) noexcept;
    ~Client() noexcept;

    void run(void);

  private:

    sockaddr_in createAddress(const std::string_view ip, const std::string_view port) const noexcept;
    int createTcpSocket(void) const noexcept;
    int createUdpSocket(void) const noexcept;

    void fetchOrderbooks(void);
    void syncSequences(void);
    void updateOrderbooks(void);

    void sendLogin(void) const;
    void recvLogin(void);
    void recvSnapshot(void);
    void sendLogout(void) const;

    void processSnapshots(const char *restrict buffer, const uint16_t length);
    void processMessageBlocks(const char *restrict buffer, uint16_t blocks_count);

    void handleSnapshotCompletion(const MessageData &data);

    static MessageHandler message_handler;
    const std::string username;
    const std::string password;
    const sockaddr_in glimpse_address;
    const sockaddr_in multicast_address;
    const sockaddr_in rewind_address;
    const sockaddr_in bind_address_tcp;
    const sockaddr_in bind_address_udp;
    const int tcp_sock_fd;
    const int udp_sock_fd;
    uint64_t sequence_number;
    enum Status { CONNECTING, FETCHING, UPDATING } status;
};