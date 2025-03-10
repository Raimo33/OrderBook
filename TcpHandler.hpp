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
    void request_snapshot(void);

  private:
    struct sockaddr_in glimpse_address;
    int fd;
    State state;
    OrderBook *order_book;
    std::chrono::time_point<std::chrono::steady_clock> last_outgoing;
    std::chrono::time_point<std::chrono::steady_clock> last_incoming;
};