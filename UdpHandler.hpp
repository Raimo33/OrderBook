#pragma once

#include <array>
#include <chrono>

class OrderBook;
class ClientConfig;
class ServerConfig;

class UdpHandler
{
  public:
    UdpHandler(const ClientConfig &client_conf, const ServerConfig &server_conf, OrderBook &order_book);
    ~UdpHandler(void);

    UdpHandler &operator=(const UdpHandler &other);

    void accumulate_updates(void);
    void process_updates(void);

  private:
    int fd;
    struct sockaddr_in multicast_address;
    struct sockaddr_in rewind_address;
    struct ip_mreq mreq;
    OrderBook *order_book;
    std::chrono::time_point<std::chrono::steady_clock> last_received;
    //TODO simple queue circular buffer for packets O(1) insert and remove, O(nlog n) sorting. prefixed size (MAX_PACKETS_IN_FLIGHT)
};