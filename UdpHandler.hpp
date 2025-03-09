#pragma once

class OrderBook;

class UdpHandler
{
  public:
    UdpHandler(struct sockaddr_in& addr, OrderBook& order_book);
    ~UdpHandler(void);

    void accumulate_updates(void);
    void process_updates(void);

  private:
    int fd;
    struct ip_mreq mreq;
    OrderBook& order_book;
    //TODO simple queue circular buffer for packets O(1) insert and remove, O(nlog n) sorting. prefixed size (MAX_PACKETS_IN_FLIGHT)
};