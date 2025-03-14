#pragma once

#include <cstdint>
#include <string>
#include <string_view>
#include <netinet/in.h>
#include <memory>
#include <chrono>

#include "Config.hpp"
#include "OrderBook.hpp"
#include "Packets.hpp"

class Client
{
  public:
    Client(const Config &config);
    ~Client(void);

    void run(void);

  private:
    
    SoupBinTCPPacket create_login_request(const ClientConfig &client_conf) const noexcept;
    SoupBinTCPPacket create_tcp_client_heartbeat(void) const noexcept;
    SoupBinTCPPacket create_logout_request(void) const noexcept;
    sockaddr_in create_address(const std::string_view ip, const uint16_t port) const noexcept;
    ip_mreq create_mreq(const std::string_view bind_address) const noexcept;
    int create_tcp_socket(void) const noexcept;
    int create_udp_socket(void) const noexcept;
    int create_timer_fd(const std::chrono::milliseconds &timeout) const noexcept;
    void bind_socket(const int fd, const std::string_view ip, const uint16_t port) const noexcept;
    void bind_epoll(void) const noexcept;

    void fetch_snapshot(const uint32_t event_mask);
    void accumulate_new_messages(const uint32_t event_mask);
    void process_live_updates(const uint32_t event_mask);
    void handle_tcp_heartbeat_timeout(const uint32_t event_mask);
    void handle_udp_heartbeat_timeout(const uint32_t event_mask);

    bool send_tcp_packet(const SoupBinTCPPacket &packet);
    bool recv_login(void);
    bool recv_snapshot(void);

    bool process_message_blocks(const std::vector<char> &buffer);

    void handle_new_order(const MessageBlock &block);
    void handle_deleted_order(const MessageBlock &block);

    Config config;
    uint8_t server_config_idx;
    bool orderbook_ready;
    const SoupBinTCPPacket login_request;
    const SoupBinTCPPacket tcp_client_heartbeat;
    const SoupBinTCPPacket logout_request;
    OrderBook order_book;
    const sockaddr_in glimpse_address;
    const sockaddr_in multicast_address;
    const sockaddr_in rewind_address;
    const ip_mreq mreq;
    const int tcp_sock_fd;
    const int udp_sock_fd;
    const int timer_fd;
    const int epoll_fd;
    uint64_t sequence_number;
    std::chrono::steady_clock::time_point last_incoming;
    std::chrono::steady_clock::time_point last_outgoing;
};