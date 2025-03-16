/*================================================================================

File: Client.cpp                                                                
Creator: Claudio Raimondi                                                       
Email: claudio.raimondi@pm.me                                                   

created at: 2025-03-08 15:48:16                                                 
last edited: 2025-03-08 21:24:05                                                

================================================================================*/

#include <arpa/inet.h>
#include <netinet/tcp.h>
#include <sys/poll.h>
#include <sys/timerfd.h>
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
  server_config(config.server_confs[0]),
  order_book(),
  glimpse_address(create_address(server_config.glimpse_endpoint.ip, server_config.glimpse_endpoint.port)),
  multicast_address(create_address(server_config.multicast_endpoint.ip, server_config.multicast_endpoint.port)),
  rewind_address(create_address(server_config.rewind_endpoint.ip, server_config.rewind_endpoint.port)),
  mreq(create_mreq(config.client_conf.bind_address)),
  tcp_sock_fd(create_tcp_socket()),
  udp_sock_fd(create_udp_socket()),
  timer_fd(create_timer_fd(50ms)),
  orderbook_ready(false),
  last_incoming(std::chrono::steady_clock::now()),
  last_outgoing(std::chrono::steady_clock::now()),
  sequence_number(0)
{
  bind_socket(tcp_sock_fd, config.client_conf.bind_address, config.client_conf.tcp_port);
  bind_socket(udp_sock_fd, config.client_conf.bind_address, config.client_conf.udp_port);

  connect(tcp_sock_fd, reinterpret_cast<const sockaddr *>(&glimpse_address), sizeof(glimpse_address));
}

COLD sockaddr_in Client::create_address(const std::string_view ip, const uint16_t port) const noexcept
{
  return {
    .sin_family = AF_INET,
    .sin_port = utils::bswap16(port),
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
  setsockopt(sock_fd, SOL_SOCKET, SO_ZEROCOPY, &enable, sizeof(enable));

  return sock_fd;
}

COLD int Client::create_udp_socket(void) const noexcept
{
  const int sock_fd = socket(AF_INET, SOCK_DGRAM | SOCK_NONBLOCK, 0);

  constexpr int enable = 1;
  constexpr int priority = 255;
  setsockopt(sock_fd, SOL_SOCKET, SO_ZEROCOPY, &enable, sizeof(enable));
  setsockopt(sock_fd, SOL_SOCKET, SO_PRIORITY, &priority, sizeof(priority));
  setsockopt(sock_fd, IPPROTO_IP, IP_ADD_MEMBERSHIP, &mreq, sizeof(mreq));
  setsockopt(sock_fd, SOL_SOCKET, SO_EXCLUSIVEADDRUSE, &enable, sizeof(enable));
  //TODO SO_RCVBUF (not < than MTU)
  //TODO SO_BINDTODEVICE

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
  bind_addr.sin_port = utils::bswap16(port);
  inet_pton(AF_INET, ip.data(), &bind_addr.sin_addr);

  bind(fd, reinterpret_cast<sockaddr *>(&bind_addr), sizeof(bind_addr));
}

Client::~Client(void)
{
  close(tcp_sock_fd);
  close(udp_sock_fd);
  close(timer_fd);
}

//TODO overhead of using unordered_map
void Client::run(void)
{
  using Handler = void (Client::*)(const uint16_t);

  {
    const std::unordered_map<int, Handler> handlers = {
      { tcp_sock_fd, &Client::fetch_snapshot },
      { udp_sock_fd, &Client::accumulate_new_messages },
      { timer_fd, &Client::handle_tcp_heartbeat_timeout }
    };

    pollfd fds[] = {
      { tcp_sock_fd, POLLIN | POLLOUT | POLLERR | POLLHUP | POLLRDHUP, 0 },
      { udp_sock_fd, POLLIN | POLLERR | POLLHUP | POLLRDHUP, 0 },
      { timer_fd, POLLIN, 0 }
    };
    constexpr nfds_t n_fds = sizeof(fds) / sizeof(fds[0]);
    
    while (!orderbook_ready)
    {
      poll(fds, n_fds, -1);

      for (auto &fd : fds)
        (this->*handlers.at(fd.fd))(fd.revents);
    }
  }

  {
    const std::unordered_map<int, Handler> handlers = {
      { udp_sock_fd, &Client::process_live_updates },
      { timer_fd, &Client::handle_udp_heartbeat_timeout }
    };

    pollfd fds[] = {
      { udp_sock_fd, POLLIN | POLLERR | POLLHUP | POLLRDHUP, 0 },
      { timer_fd, POLLIN, 0 }
    };
    constexpr nfds_t n_fds = sizeof(fds) / sizeof(fds[0]);

    while (true)
    {
      poll(fds, n_fds, -1);

      for (auto &fd : fds)
        (this->*handlers.at(fd.fd))(fd.revents);
    }
  }
}

COLD void Client::fetch_snapshot(const uint16_t event_mask)
{
  static uint8_t state = 0;

  if (UNLIKELY(event_mask & (POLLERR | POLLHUP | POLLRDHUP)))
    utils::throw_exception("Server disconnected");

  switch (state)
  {
    case 0:
      state += send_login(event_mask);
      break;
    case 1:
      state += recv_login(event_mask);
      break;
    case 2:
      state += recv_snapshot(event_mask);
      break;
    case 3:
      orderbook_ready = send_logout(event_mask);
      state *= !orderbook_ready;
      break;
  }
}

COLD void Client::accumulate_new_messages(const uint16_t event_mask)
{
  if (UNLIKELY(event_mask & (POLLERR | POLLHUP | POLLRDHUP)))
    utils::throw_exception("Server disconnected");

  //TODO recvmsg with address specified
  {
    //read packet
    //put in queue (by sorting by sequence number)
  }
}

HOT void Client::process_live_updates(const uint16_t event_mask)
{
  if (UNLIKELY(event_mask & (POLLERR | POLLHUP | POLLRDHUP)))
    utils::throw_exception("Server disconnected");

  {
    //read packet
    //put orders in queue (by sorting by sequence number)
    //empty the queue as long as you extract packets in order
  }
}

HOT void Client::handle_tcp_heartbeat_timeout(UNUSED const uint16_t event_mask)
{
  const auto now = std::chrono::steady_clock::now();
  
  uint64_t _;
  utils::safe_read(timer_fd, reinterpret_cast<char *>(&_), sizeof(_));

  if (now - last_outgoing > 1s)
    send_hearbeat();
  if (UNLIKELY(now - last_incoming > 50ms))
    utils::throw_exception("Server timeout");
}

COLD bool Client::send_login(const uint16_t event_mask)
{
  if (!(event_mask & POLLOUT))
    return false;

  SoupBinTCPPacket packet{};

  packet.length = sizeof(packet.type) + sizeof(packet.data.login_request);
  packet.type = 'L';

  auto &login = packet.data.login_request;

  std::memcpy(login.username, config.client_conf.username.c_str(), sizeof(login.username));
  std::memcpy(login.password, config.client_conf.password.c_str(), sizeof(login.password));
  std::memset(login.requested_session, ' ', sizeof(login.requested_session));
  login.requested_sequence_number[0] = '1';

  constexpr uint16_t packet_size = sizeof(packet.length) + sizeof(packet.type) + sizeof(login);
  utils::safe_send(tcp_sock_fd, reinterpret_cast<const char *>(&packet), packet_size);
  last_outgoing = std::chrono::steady_clock::now();

  return true;
}

COLD bool Client::recv_login(const uint16_t event_mask)
{
  if (!(event_mask & POLLIN))
    return false;

  SoupBinTCPPacket packet;
  constexpr uint16_t header_size = sizeof(packet.length) + sizeof(packet.type);

  utils::safe_recv(tcp_sock_fd, reinterpret_cast<char *>(&packet), header_size);
  last_incoming = std::chrono::steady_clock::now();

  switch (packet.type)
  {
    case 'H':
      return recv_login(event_mask);
    case 'J':
      utils::throw_exception("Login rejected");
      UNREACHABLE;
    case 'A':
    {
      const uint16_t payload_size = utils::bswap32(packet.length) - sizeof(packet.type);
      utils::assert(payload_size == sizeof(packet.data.login_acceptance), "Unexpected login acceptance size");
      utils::safe_recv(tcp_sock_fd, reinterpret_cast<char *>(&packet.data.login_acceptance), payload_size);
      sequence_number = utils::atoul(packet.data.login_acceptance.sequence_number);
      return true;
    }
    default:
      utils::throw_exception("Invalid packet type");
  }

  UNREACHABLE;
}

bool Client::recv_snapshot(const uint16_t event_mask)
{
  if (!(event_mask & POLLIN))
    return false;

  SoupBinTCPPacket packet;
  constexpr uint16_t header_size = sizeof(packet.length) + sizeof(packet.type);

  utils::safe_recv(tcp_sock_fd, reinterpret_cast<char *>(&packet), header_size);
  last_incoming = std::chrono::steady_clock::now();

  switch (packet.type)
  {
    case 'H':
      return recv_snapshot(event_mask);
    case 'S':
    {
      const uint16_t payload_size = utils::bswap32(packet.length) - sizeof(packet.type);
      std::vector<char> buffer(payload_size);
      utils::safe_recv(tcp_sock_fd, buffer.data(), payload_size);
      return process_message_blocks(buffer);
    }
    default:
      utils::throw_exception("Invalid packet type");
  }

  UNREACHABLE;
}

COLD bool Client::send_logout(const uint16_t event_mask)
{
  if (!(event_mask & POLLOUT))
    return false;

  constexpr SoupBinTCPPacket packet = {
    .length = 1,
    .type = 'Z',
    .data{}
  };
  constexpr uint16_t packet_size = sizeof(packet.length) + sizeof(packet.type);

  utils::safe_send(tcp_sock_fd, reinterpret_cast<const char *>(&packet), packet_size);
  last_outgoing = std::chrono::steady_clock::now();

  return true;
}

HOT void Client::send_hearbeat(void)
{
  constexpr SoupBinTCPPacket packet = {
    .length = 1,
    .type = 'H',
    .data{}
  };
  constexpr uint16_t packet_size = sizeof(packet.length) + sizeof(packet.type);

  utils::safe_send(tcp_sock_fd, reinterpret_cast<const char *>(&packet), packet_size);
  last_outgoing = std::chrono::steady_clock::now();
}

bool Client::process_message_blocks(const std::vector<char> &buffer)
{
  auto it = buffer.cbegin();
  const auto end = buffer.cend();

  static const std::unordered_map<char, uint16_t> message_sizes = {
    {'T', sizeof(MessageBlock::Seconds)},
    {'R', sizeof(MessageBlock::SeriesInfoBasic)},
    {'M', sizeof(MessageBlock::SeriesInfoBasicCombination)},
    {'L', sizeof(MessageBlock::TickSizeData)},
    {'S', sizeof(MessageBlock::SystemEventData)},
    {'O', sizeof(MessageBlock::TradingStatusData)},
    {'A', sizeof(MessageBlock::NewOrder)},
    {'E', sizeof(MessageBlock::ExecutionNotice)},
    {'C', sizeof(MessageBlock::ExecutionNoticeWithTradeInfo)},
    {'D', sizeof(MessageBlock::DeletedOrder)},
    {'Z', sizeof(MessageBlock::EP)},
    {'G', sizeof(MessageBlock::SnapshotCompletion)}
  };

  while (it != end)
  {
    const MessageBlock &block = *reinterpret_cast<const MessageBlock *>(&*it);
    const uint16_t block_length = utils::bswap16(block.length);

    utils::assert(message_sizes.at(block.type) == block_length, "Unexpected message block length");

    switch (block.type)
    {
      case 'A':
        handle_new_order(block);
        break;
      case 'D':
        handle_deleted_order(block);
        break;
      case 'G':
      {
        const uint64_t sequence_number = utils::atoul(block.data.snapshot_completion.sequence);
        utils::assert(sequence_number == ++this->sequence_number, "Unexpected sequence number");
        return true;
      }
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
  const int32_t price = utils::bswap32(new_order.price);
  const uint64_t volume = utils::bswap32(new_order.quantity);

  if (price == INT32_MIN)
    order_book.executeOrder(side, volume);
  else
    order_book.addOrder(side, price, volume);
}

//TODO find a way to efficiently remove orders
HOT void Client::handle_deleted_order(const MessageBlock &block)
{
  (void)block;
  // const auto &deleted_order = block.data.deleted_order;
  // const OrderBook::Side side = static_cast<OrderBook::Side>(deleted_order.side == 'S');
  // const uint32_t price = utils::bswap32(deleted_order.price);
  // const uint64_t volume = utils::bswap32(deleted_order.quantity);

  // order_book.removeOrder(side, price, volume);
}

HOT void Client::handle_udp_heartbeat_timeout(UNUSED const uint16_t event_mask)
{
  const auto &now = std::chrono::steady_clock::now();

  uint64_t _;
  utils::safe_read(timer_fd, reinterpret_cast<char *>(&_), sizeof(_));

  if (now - last_incoming > 50ms)
    utils::throw_exception("Server timeout");
}