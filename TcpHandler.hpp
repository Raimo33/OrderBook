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

    enum State : uint8_t {
      DISCONNECTED      = 0,
      CONNECTED         = 1,
      LOGIN_SENT        = 2,
      LOGIN_RECEIVED    = 3,
      SNAPSHOT_RECEIVED = 4,
      LOGGED_OUT        = 5
    };

    State get_state(void) const;
    int get_fd(void) const;

    void request_snapshot(const uint32_t event_mask);

  private:
    const sockaddr_in init_glimpse_address(const ServerConfig &server_conf);
    const int init_socket(void);
    const SoupBinTCPPacket<LoginRequest> init_login_request(const ClientConfig &client_conf);
    const SoupBinTCPPacket<LogoutRequest> init_logout_request(void);
    const SoupBinTCPPacket<UserHeartbeat> init_client_heartbeat(void);

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