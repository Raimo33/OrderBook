#pragma once

#include <array>
#include <chrono>

class OrderBook;

class UdpHandler
{
  public:
    UdpHandler(OrderBook& order_book);
    ~UdpHandler(void);

    void accumulate_updates(void);
    void process_updates(void);

  private:
    std::array<struct sockaddr_in, 2> multicast_addresses;
    std::array<struct sockaddr_in, 2> rewind_addresses;
    int fd;
    struct ip_mreq mreq;
    OrderBook& order_book;
    std::chrono::time_point<std::chrono::steady_clock> last_received;
    //TODO simple queue circular buffer for packets O(1) insert and remove, O(nlog n) sorting. prefixed size (MAX_PACKETS_IN_FLIGHT)
};