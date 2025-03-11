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

    int get_sock_fd(void) const noexcept;
    int get_timer_fd(void) const noexcept;

    void accumulate_updates(const uint32_t event_mask);
    void process_updates(const uint32_t event_mask);
    void handle_heartbeat_timeout(const uint32_t event_mask);

  private:

    const ip_mreq create_mreq(const std::string_view bind_address) const noexcept;
    const int create_socket(void) const noexcept;

    int sock_fd;
    int timer_fd;
    sockaddr_in multicast_address;
    sockaddr_in rewind_address;
    ip_mreq mreq;
    OrderBook *order_book;
    std::chrono::time_point<std::chrono::steady_clock> last_incoming;
    //TODO simple queue circular buffer for packets O(1) insert and remove, O(nlog n) sorting. prefixed size (MAX_PACKETS_IN_FLIGHT)
};