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
    void send_login(const uint32_t event_mask);
    void recv_login(const uint32_t event_mask);
    void recv_snapshot(const uint32_t event_mask);
    void send_logout(const uint32_t event_mask);
    void send_hearbeat(const uint32_t event_mask);

    sockaddr_in glimpse_address;
    int fd;
    State state;
    OrderBook *order_book;
    std::string username;
    std::string password;
    std::chrono::time_point<std::chrono::steady_clock> last_outgoing;
    std::chrono::time_point<std::chrono::steady_clock> last_incoming;
};