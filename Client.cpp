/*================================================================================

File: Client.cpp                                                                
Creator: Claudio Raimondi                                                       
Email: claudio.raimondi@pm.me                                                   

created at: 2025-03-08 15:48:16                                                 
last edited: 2025-03-08 21:24:05                                                

================================================================================*/

#include <arpa/inet.h>
#include <netinet/tcp.h>
#include <sys/epoll.h>
#include <sys/timerfd.h>
#include <unistd.h>
#include <cstring>
#include <array>
#include <unordered_map>

#include "Client.hpp"
#include "Config.hpp"
#include "utils.hpp"
#include "macros.hpp"

using namespace std::chrono_literals;

Client::Client(const Config &config) :
  config(config),
  server_config_idx(0),
  orderbook_ready(false),
  login_request(create_login_request(config.client_conf)),
  tcp_client_heartbeat(create_tcp_client_heartbeat()),
  logout_request(create_logout_request()),
  order_book(),
  glimpse_address(create_address(config.server_confs[server_config_idx].glimpse_endpoint.ip, config.server_confs[server_config_idx].glimpse_endpoint.port)),
  multicast_address(create_address(config.server_confs[server_config_idx].multicast_endpoint.ip, config.server_confs[server_config_idx].multicast_endpoint.port)),
  rewind_address(create_address(config.server_confs[server_config_idx].rewind_endpoint.ip, config.server_confs[server_config_idx].rewind_endpoint.port)),
  mreq(create_mreq(config.client_conf.bind_address)),
  tcp_sock_fd(create_tcp_socket()),
  udp_sock_fd(create_udp_socket()),
  timer_fd(create_timer_fd(50ms)),
  epoll_fd(epoll_create1(0)),
  last_incoming(std::chrono::steady_clock::now()),
  last_outgoing(std::chrono::steady_clock::now()),
  sequence_number(0)
{
  bind_socket(tcp_sock_fd, config.client_conf.bind_address, config.client_conf.tcp_port);
  bind_socket(udp_sock_fd, config.client_conf.bind_address, config.client_conf.udp_port);
  bind_epoll();
}

COLD SoupBinTCPPacket Client::create_login_request(const ClientConfig &client_conf) const noexcept
{
  SoupBinTCPPacket packet{};

  packet.length = sizeof(packet.type) + sizeof(packet.login_request);
  packet.type = 'L';

  auto &login = packet.login_request;

  std::memcpy(login.username, client_conf.username.c_str(), sizeof(login.username));
  std::memcpy(login.password, client_conf.password.c_str(), sizeof(login.password));
  std::memset(login.requested_session, ' ', sizeof(login.requested_session));
  login.requested_sequence_number[0] = '1';

  return packet;
}

COLD SoupBinTCPPacket Client::create_tcp_client_heartbeat(void) const noexcept
{
  constexpr SoupBinTCPPacket packet = {
    .length = 1,
    .type = 'H',
    .client_heartbeat{}
  };

  return packet;
}

COLD SoupBinTCPPacket Client::create_logout_request(void) const noexcept
{
  constexpr SoupBinTCPPacket packet = {
    .length = 1,
    .type = 'L',
    .logout_request{}
  };

  return packet;
}

COLD sockaddr_in Client::create_address(const std::string_view ip, const uint16_t port) const noexcept
{
  return {
    .sin_family = AF_INET,
    .sin_port = htons(port),
    .sin_addr = {
      .s_addr = inet_addr(ip.data())
    },
    .sin_zero{}
  };
}

COLD ip_mreq Client::create_mreq(const std::string_view bind_address) const noexcept
{
  return {
    .imr_multiaddr = multicast_address.sin_addr,
    .imr_interface = inet_addr(bind_address.data())
  };
}

COLD int Client::create_tcp_socket(void) const noexcept
{
  const int sock_fd = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0);

  constexpr int enable = 1;
  setsockopt(sock_fd, IPPROTO_TCP, TCP_FASTOPEN, &enable, sizeof(enable));
  setsockopt(sock_fd, IPPROTO_TCP, TCP_NODELAY, &enable, sizeof(enable));

  return sock_fd;
}

COLD int Client::create_udp_socket(void) const noexcept
{
  const int sock_fd = socket(AF_INET, SOCK_DGRAM | SOCK_NONBLOCK, 0);

  constexpr int enable = 1;
  setsockopt(sock_fd, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(enable));
  setsockopt(sock_fd, IPPROTO_IP, IP_ADD_MEMBERSHIP, &mreq, sizeof(mreq));

  return sock_fd;
}

COLD int create_timer_fd(const std::chrono::milliseconds interval)
{
  const int timer_fd = timerfd_create(CLOCK_MONOTONIC, 0);

  const auto seconds = std::chrono::duration_cast<std::chrono::seconds>(interval);
  const auto nanoseconds = interval - seconds;

  const struct itimerspec timer_spec = {
    .it_interval = { .tv_sec = seconds.count(), .tv_nsec = nanoseconds.count() },
    .it_value = { .tv_sec = seconds.count(), .tv_nsec = nanoseconds.count() }
  };

  timerfd_settime(timer_fd, 0, &timer_spec, nullptr);

  return timer_fd;
}

COLD void Client::bind_socket(const int fd, const std::string_view ip, const uint16_t port) const noexcept
{
  sockaddr_in bind_addr;
  bind_addr.sin_family = AF_INET;
  bind_addr.sin_port = htons(port);
  inet_pton(AF_INET, ip.data(), &bind_addr.sin_addr);

  bind(fd, reinterpret_cast<sockaddr *>(&bind_addr), sizeof(bind_addr));
}

COLD void Client::bind_epoll(void) const noexcept
{
  constexpr uint32_t network_events_mask = EPOLLIN | EPOLLERR | EPOLLHUP | EPOLLRDHUP;
  constexpr uint32_t timer_events_mask = EPOLLIN;

  epoll_event events[4] = {
    { .events = network_events_mask, .data = {.fd = tcp_sock_fd} },
    { .events = network_events_mask, .data = {.fd = udp_sock_fd} },
    { .events = timer_events_mask, .data = {.fd = timer_fd} }
  };

  epoll_ctl(epoll_fd, EPOLL_CTL_ADD, tcp_sock_fd, &events[0]);
  epoll_ctl(epoll_fd, EPOLL_CTL_ADD, udp_sock_fd, &events[1]);
  epoll_ctl(epoll_fd, EPOLL_CTL_ADD, timer_fd, &events[2]);
}

Client::~Client(void)
{
  close(tcp_sock_fd);
  close(udp_sock_fd);
  close(timer_fd);
  close(epoll_fd);
}

void Client::run(void)
{
  struct Entry { int fd; void (Client::*handler)(const uint32_t); };

  {
    const Entry handlers[] = {
      {tcp_sock_fd, &Client::fetch_snapshot},
      {udp_sock_fd, &Client::accumulate_new_messages},
      {timer_fd, &Client::handle_tcp_heartbeat_timeout}
    };
    
    while (!orderbook_ready)
    {
      std::array<epoll_event, 4> events;
      uint8_t n = epoll_wait(epoll_fd, events.data(), events.size(), -1);

      while (n--)
      {
        const epoll_event& event = events[n];
        (this->*handlers[event.data.fd].handler)(event.events);
      }
    }
  }

  epoll_ctl(epoll_fd, EPOLL_CTL_DEL, tcp_sock_fd, nullptr);

  {
    const Entry handlers[] = {
      {udp_sock_fd, &Client::process_live_updates},
      {timer_fd, &Client::handle_udp_heartbeat_timeout}
    };

    while (true)
    {
      std::array<epoll_event, 2> events;
      uint8_t n = epoll_wait(epoll_fd, events.data(), events.size(), -1);

      while (n--)
      {
        const epoll_event event = events[n];
        (this->*handlers[event.data.fd].handler)(event.events);
      }

      // engine.print_orderbook();
    }
  }
}

COLD void Client::fetch_snapshot(const uint32_t event_mask)
{
  static uint8_t state = 0;

  if (event_mask & (EPOLLERR | EPOLLHUP | EPOLLRDHUP))
    utils::throw_exception("Error in tcp socket");

  switch (state)
  {
    case 0:
      state += connect(tcp_sock_fd, reinterpret_cast<const sockaddr *>(&glimpse_address), sizeof(glimpse_address));
      break;
    case 1:
      state += send_tcp_packet(login_request);
      break;
    case 2:
      state += recv_login();
      break;
    case 3:
      state += recv_snapshot();
      break;
    case 4:
      state *= !send_tcp_packet(logout_request);
      orderbook_ready = (state == 0);
      break;
  }
}

COLD void Client::accumulate_new_messages(const uint32_t event_mask)
{
  if (event_mask & (EPOLLERR | EPOLLHUP | EPOLLRDHUP))
    utils::throw_exception("Error in udp socket");

  //TODO state machine, use connect() to bind to the multicast address because i use normal send()
  {
    //read packet
    //put in queue (by sorting by sequence number)
  }
}

HOT void Client::process_live_updates(const uint32_t event_mask)
{
  if (event_mask & (EPOLLERR | EPOLLHUP | EPOLLRDHUP))
    utils::throw_exception("Error in udp socket");

  {
    //read packet
    //put orders in queue (by sorting by sequence number)
    //empty the queue as long as you extract packets in order
  }
}

HOT void Client::handle_tcp_heartbeat_timeout(UNUSED const uint32_t event_mask)
{
  const auto now = std::chrono::steady_clock::now();
  
  uint64_t _;
  read(timer_fd, &_, sizeof(_));

  if (now - last_outgoing > 1s)
    send_tcp_packet(tcp_client_heartbeat);
  if (UNLIKELY(now - last_incoming > 50ms))
    utils::throw_exception("Server timeout");
}

COLD bool Client::send_tcp_packet(const SoupBinTCPPacket &packet)
{
  const char *buffer = reinterpret_cast<const char *>(&packet);
  const uint16_t total_size = sizeof(packet.length) + packet.length;
  static uint16_t sent_bytes = 0;

  sent_bytes += utils::try_tcp_send(tcp_sock_fd, buffer + sent_bytes, total_size - sent_bytes);
  if (sent_bytes < total_size)
    return false;

  last_outgoing = std::chrono::steady_clock::now();
  sent_bytes = 0;
  return true;
}

COLD bool Client::recv_login(void)
{
  static enum State { HEADER, PAYLOAD } state = HEADER;
  static SoupBinTCPPacket packet;
  static uint16_t bytes_read = 0;

  switch (state)
  {
    case HEADER:
      constexpr uint16_t header_size = sizeof(packet.length) + sizeof(packet.type);
      bytes_read += utils::try_recv(tcp_sock_fd, reinterpret_cast<char *>(&packet) + bytes_read, header_size - bytes_read);
      if (bytes_read < header_size)
        return false;
      bytes_read = 0;
      state = PAYLOAD;
      [[fallthrough]];
    case PAYLOAD:
      switch (packet.type)
      {
        case 'H':
          last_incoming = std::chrono::steady_clock::now();
          state = HEADER;
          return false;
        case 'J':
          //TODO read the reason
          utils::throw_exception("Login rejected");
        case 'A':
          const uint16_t payload_size = ntohl(packet.length) - sizeof(packet.type);
          utils::assert(payload_size == sizeof(packet.login_acceptance), "Unexpected login acceptance size");
          bytes_read += utils::try_recv(tcp_sock_fd, reinterpret_cast<char *>(&packet.login_acceptance) + bytes_read, payload_size - bytes_read);
          if (bytes_read < payload_size)
            return false;
          sequence_number = utils::atoul(packet.login_acceptance.sequence_number);
          state = HEADER;
          bytes_read = 0;
          return true;
        default:
          utils::throw_exception("Invalid packet type");
      }
  }
}

bool Client::recv_snapshot(void)
{
  static enum State { HEADER, PAYLOAD } state = HEADER;
  static uint16_t bytes_read = 0;
  static SoupBinTCPPacket packet;
  static std::vector<char> buffer;

  switch (state)
  {
    case HEADER:
      constexpr uint16_t header_size = sizeof(packet.length) + sizeof(packet.type);
      bytes_read += utils::try_recv(tcp_sock_fd, reinterpret_cast<char *>(&packet) + bytes_read, header_size - bytes_read);
      if (bytes_read < header_size)
        return false;
      bytes_read = 0;
      state = PAYLOAD;
      [[fallthrough]];
    case PAYLOAD:
      switch (packet.type)
      {
        case 'H':
          last_incoming = std::chrono::steady_clock::now();
          state = HEADER;
          return false;
        case 'S':
          const uint16_t payload_size = ntohl(packet.length) - sizeof(packet.type);
          buffer.resize(payload_size);
          bytes_read += utils::try_recv(tcp_sock_fd, buffer.data() + bytes_read, payload_size - bytes_read);
          if (bytes_read < payload_size)
            return false;
          const bool snapshot_complete = process_message_blocks(buffer);
          buffer.clear();
          bytes_read = 0;
          state = HEADER;
          return snapshot_complete;
        default:
          utils::throw_exception("Invalid packet type");
      }
  }
}

bool Client::process_message_blocks(const std::vector<char> &buffer)
{
  auto it = buffer.cbegin();
  const auto end = buffer.cend();

  static const std::unordered_map<char, uint16_t> message_sizes = {
    {'T', sizeof(MessageBlock::data.seconds)},
    {'R', sizeof(MessageBlock::data.series_info_basic)},
    {'M', sizeof(MessageBlock::data.series_info_basic_combination)},
    {'L', sizeof(MessageBlock::data.tick_size_data)},
    {'S', sizeof(MessageBlock::data.system_event_data)},
    {'O', sizeof(MessageBlock::data.trading_status_data)},
    {'A', sizeof(MessageBlock::data.new_order)},
    {'E', sizeof(MessageBlock::data.execution_notice)},
    {'C', sizeof(MessageBlock::data.execution_notice_with_trade_info)},
    {'D', sizeof(MessageBlock::data.deleted_order)},
    {'Z', sizeof(MessageBlock::data.ep)}
  };

  while (it != end)
  {
    const MessageBlock &block = *reinterpret_cast<const MessageBlock *>(&*it);

    utils::assert(message_sizes.at(block.type) == block.length, "Unexpected message block length");

    switch (block.type)
    {
      case 'A':
        handle_new_order(block);
        break;
      case 'D':
        handle_deleted_order(block);
        break;
      case 'G':
        const uint64_t sequence_number = utils::atoul(block.data.snapshot_completion.sequence);
        utils::assert(sequence_number == this->sequence_number++, "Unexpected sequence number");
        return true;
      default:
        break;
    }

    sequence_number++;
    std::advance(it, block.length);
  }

  return false;
}

HOT void Client::handle_new_order(const MessageBlock &block)
{
  const auto &new_order = block.data.new_order;
  const OrderBook::Side side = static_cast<OrderBook::Side>(new_order.side == 'S');
  const int32_t price = ntohl(new_order.price);
  const uint64_t volume = ntohl(new_order.quantity);

  if (price == INT32_MIN)
    order_book.executeOrder(side, volume);
  else
    order_book.addOrder(side, price, volume);
}

HOT void Client::handle_deleted_order(const MessageBlock &block)
{
  const auto &deleted_order = block.data.deleted_order;
  const OrderBook::Side side = static_cast<OrderBook::Side>(deleted_order.side == 'S');
  const uint32_t price = ntohl(deleted_order.price);
  const uint64_t volume = ntohl(deleted_order.quantity);

  order_book.removeOrder(side, price, volume);
}

HOT void Client::handle_udp_heartbeat_timeout(UNUSED const uint32_t event_mask)
{
  const auto &now = std::chrono::steady_clock::now();

  uint64_t _;
  read(timer_fd, &_, sizeof(_));

  if (now - last_incoming > 50ms)
    utils::throw_exception("Server timeout");
}