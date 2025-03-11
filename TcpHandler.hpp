#pragma once

#include <array>
#include <chrono>

#include "TcpPackets.hpp"

class OrderBook;
class ClientConfig;
class ServerConfig;

class TcpHandler
{
  public:

    TcpHandler(const ClientConfig &client_conf, const ServerConfig &server_conf, OrderBook &order_book);
    ~TcpHandler(void);

    TcpHandler& operator=(const TcpHandler& other);

    inline bool has_finished(void) const noexcept;
    inline int  get_sock_fd(void) const noexcept;
    inline int  get_timer_fd(void) const noexcept;

    void request_snapshot(const uint32_t event_mask);
    void handle_heartbeat_timeout(const uint32_t event_mask);

  private:

    int create_socket(void) const noexcept;

    const SoupBinTCPPacket<LoginRequest>  create_login_request(const ClientConfig &client_conf) const noexcept;
    const SoupBinTCPPacket<LogoutRequest> create_logout_request(void) const noexcept;
    const SoupBinTCPPacket<UserHeartbeat> create_user_heartbeat(void) const noexcept;

    bool send_login(void);
    bool recv_login(void);
    bool recv_snapshot(void);
    bool send_logout(void);
    bool send_hearbeat(void);

    uint8_t state;
    OrderBook *order_book;
    const sockaddr_in glimpse_address;
    const int sock_fd;
    const int timer_fd;
    const SoupBinTCPPacket<LoginRequest>  login_request;
    const SoupBinTCPPacket<LogoutRequest> logout_request;
    const SoupBinTCPPacket<UserHeartbeat> user_heartbeat;
    std::chrono::time_point<std::chrono::steady_clock> last_outgoing;
    std::chrono::time_point<std::chrono::steady_clock> last_incoming;
};

#include "TcpHandler.inl"