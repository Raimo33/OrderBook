#pragma once

class OrderBook;

class TcpHandler
{
  public:
    TcpHandler(struct sockaddr_in& addr, OrderBook& order_book);
    ~TcpHandler(void);

    void request_snapshot(void);

  private:
    enum state_t : uint8_t { DISCONNECTED, LOGIN_SENT, LOGIN_RECEIVED, SNAPSHOT_RECEIVED, LOGGED_OUT };

    int fd;
    state_t state;
    OrderBook& order_book;
};