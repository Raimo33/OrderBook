#pragma once

#include <array>
#include <chrono>

class OrderBook;
class ClientConfig;
class ServerConfig;

class TcpHandler
{
  public:
    TcpHandler(const ClientConfig &client_conf, const ServerConfig &server_conf, OrderBook &order_book);
    ~TcpHandler(void);

    TcpHandler& operator=(const TcpHandler& other);

    enum State : uint8_t { DISCONNECTED, CONNECTED, LOGIN_SENT, LOGIN_RECEIVED, SNAPSHOT_RECEIVED, LOGGED_OUT };

    State get_state(void) const;
    int get_fd(void) const;

    void request_snapshot(const uint32_t event_mask);

  private:
    const int create_socket(void) const;
    const SoupBinTCPPacket<LoginRequest> create_login_request(const ClientConfig &client_conf) const;
    const SoupBinTCPPacket<LogoutRequest> create_logout_request(void) const;
    const SoupBinTCPPacket<UserHeartbeat> create_client_heartbeat(void) const;

    bool send_login(const uint32_t event_mask);
    bool recv_login(const uint32_t event_mask);
    bool recv_snapshot(const uint32_t event_mask);
    bool send_logout(const uint32_t event_mask);
    bool send_hearbeat(const uint32_t event_mask);

    const sockaddr_in glimpse_address;
    const int sock_fd;
    State state;
    OrderBook *order_book;
    const SoupBinTCPPacket<LoginRequest> login_request;
    const SoupBinTCPPacket<LogoutRequest> logout_request;
    const SoupBinTCPPacket<UserHeartbeat> client_heartbeat;
    std::chrono::time_point<std::chrono::steady_clock> last_outgoing;
    std::chrono::time_point<std::chrono::steady_clock> last_incoming;
};