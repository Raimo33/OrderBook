#pragma once

#include <array>
#include <chrono>

class OrderBook;

class TcpHandler
{
  public:
    TcpHandler(OrderBook& order_book);
    ~TcpHandler(void);

    enum State : uint8_t { DISCONNECTED, CONNECTED, LOGIN_SENT, LOGIN_RECEIVED, SNAPSHOT_RECEIVED, LOGGED_OUT };
    
    void request_snapshot(void);
    State get_state(void) const;

  private:
    std::array<struct sockaddr_in, 2> glimpse_addresses;
    int fd;
    State state;
    OrderBook& order_book;
    std::chrono::time_point<std::chrono::steady_clock> last_outgoing;
    std::chrono::time_point<std::chrono::steady_clock> last_incoming;
};