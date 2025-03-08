#pragma once

#include <cstdint>
#include <string_view>
#include <netinet/in.h>

#include "OrderBook.hpp"

class Client
{
  public:
    Client(const std::string_view ip_str, const uint16_t port);
    ~Client(void);

    void run(void);

  private:

    enum tcp_state { DISCONNECTED, CONNECTED, ORDERBOOK_REQUESTED, ORDERBOOK_RECEIVED };

    void init_udp_socket(void);
    void init_tcp_socket(void);
    void init_epoll(void);

    //TODO care for endianess (numbers are sent in network byte order)
    void handle_udp_event(const uint32_t ev, char *buffer, const uint16_t buffer_size);
    void handle_tcp_event(const uint32_t ev, char *buffer, const uint16_t buffer_size);
    void handle_error(const int fd, const uint32_t ev);

    void free_udp_socket(void);
    void free_tcp_socket(void);
    void free_epoll(void);

    struct sockaddr_in addr;
    struct ip_mreq mreq;
    int udp_fd;
    int tcp_fd;
    int epoll_fd;
    tcp_state state;
    OrderBook order_book;
    //TODO simple queue circular buffer for packets O(1) insert and remove, O(nlog n) sorting. prefixed size (MAX_PACKETS_IN_FLIGHT)
};