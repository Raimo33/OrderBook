#include <arpa/inet.h>
#include <netinet/tcp.h>
#include <sys/epoll.h>
#include <unistd.h>
#include <stdexcept>
#include <chrono>
#include <sys/timerfd.h>
#include <cstring>

#include "TcpHandler.hpp"
#include "Config.hpp"
#include "TcpPackets.hpp"
#include "utils.hpp"
#include "macros.hpp"

using namespace std::chrono_literals;

TcpHandler::TcpHandler(const ClientConfig &client_conf, const ServerConfig &server_conf, OrderBook &order_book) :
  state(0),
  order_book(&order_book),
  glimpse_address(utils::create_address(server_conf.glimpse_endpoint.ip, server_conf.glimpse_endpoint.port)),
  sock_fd(create_socket()),
  timer_fd(utils::create_timer(50ms)),
  login_request(create_login_request(client_conf)),
  logout_request(create_logout_request()),
  user_heartbeat(create_user_heartbeat()),
  last_outgoing(std::chrono::steady_clock::now()),
  last_incoming(std::chrono::steady_clock::now())
{
  sockaddr_in bind_addr;
  bind_addr.sin_family = AF_INET;
  bind_addr.sin_port = htons(client_conf.tcp_port);
  inet_pton(AF_INET, client_conf.bind_address.c_str(), &bind_addr.sin_addr);

  bind(sock_fd, reinterpret_cast<sockaddr *>(&bind_addr), sizeof(bind_addr));
}

const SoupBinTCPPacket<LoginRequest> TcpHandler::create_login_request(const ClientConfig &client_conf) const noexcept
{
  SoupBinTCPPacket<LoginRequest> packet;

  packet.length = sizeof(LoginRequest) + sizeof(packet.type);
  packet.type = 'L';
  std::memcpy(packet.body.username, client_conf.username.c_str(), sizeof(packet.body.username));
  std::memcpy(packet.body.password, client_conf.password.c_str(), sizeof(packet.body.password));
  std::memset(packet.body.requested_session, ' ', sizeof(packet.body.requested_session));
  packet.body.requested_sequence_number[0] = '1';

  return packet;
}

const SoupBinTCPPacket<LogoutRequest> TcpHandler::create_logout_request(void) const noexcept
{
  constexpr SoupBinTCPPacket<LogoutRequest> packet = {
    .length = sizeof(packet.type),
    .type = 'O',
    .body{}
  };

  return packet;
}

const SoupBinTCPPacket<UserHeartbeat> TcpHandler::create_user_heartbeat(void) const noexcept
{
  constexpr SoupBinTCPPacket<UserHeartbeat> packet = {
    .length = sizeof(packet.type),
    .type = 'R',
    .body{}
  };

  return packet;
}

int TcpHandler::create_socket(void) const noexcept
{
  const int sock_fd = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0);

  constexpr int enable = 1;
  setsockopt(sock_fd, IPPROTO_TCP, TCP_FASTOPEN, &enable, sizeof(enable));
  setsockopt(sock_fd, IPPROTO_TCP, TCP_NODELAY, &enable, sizeof(enable));

  return sock_fd;
}

TcpHandler::~TcpHandler(void)
{
  close(sock_fd);
}

TcpHandler &TcpHandler::operator=(const TcpHandler &other)
{
  if (this == &other)
    return *this;

  state = other.state;
  order_book = other.order_book;
  last_outgoing = other.last_outgoing;
  last_incoming = other.last_incoming;

  return *this;
}

COLD void TcpHandler::request_snapshot(const uint32_t event_mask)
{
  if (event_mask & (EPOLLERR | EPOLLHUP | EPOLLRDHUP))
    utils::throw_exception("Error in tcp socket");

  switch (state)
  {
    case 0:
      state += connect(sock_fd, reinterpret_cast<const sockaddr *>(&glimpse_address), sizeof(glimpse_address));
      [[fallthrough]];
    case 1:
      state += send_login();
      [[fallthrough]];
    case 2:
      state += recv_login();
      [[fallthrough]];
    case 3:
      state += recv_snapshot();
      [[fallthrough]];
    case 4:
      state += send_logout();
      [[fallthrough]];
    case 5:
      break;
  }

  UNREACHABLE;
}

void TcpHandler::handle_heartbeat_timeout(UNUSED const uint32_t event_mask)
{
  const auto now = std::chrono::steady_clock::now();

  if (now - last_incoming > 50ms)
    utils::throw_exception("Server timeout");
  if (now - last_outgoing > 1s)
    send_hearbeat();
}

COLD bool TcpHandler::send_login(void)
{
  static const char *buffer = reinterpret_cast<const char *>(&login_request);
  constexpr uint32_t total_size = sizeof(buffer);
  static uint32_t sent_bytes = 0;

  sent_bytes += utils::try_tcp_send(sock_fd, buffer + sent_bytes, total_size - sent_bytes);
  last_outgoing = std::chrono::steady_clock::now();

  return (sent_bytes == total_size);
}

COLD bool TcpHandler::recv_login(void)
{
  static SoupBinTCPPacket<LoginAcceptance> packet;
  static char *buffer = reinterpret_cast<char *>(&packet);
  constexpr uint32_t total_size = sizeof(packet);
  static uint32_t received_bytes = 0;

  received_bytes += utils::try_recv(sock_fd, buffer + received_bytes, total_size - received_bytes);
  last_incoming = std::chrono::steady_clock::now();

  const bool is_heartbeat = (packet.type == 'H');
  received_bytes -= (is_heartbeat * sizeof(SoupBinTCPPacket<ServerHeartbeat>));

  //TODO interpret login response

  return (received_bytes == total_size);
}

bool TcpHandler::recv_snapshot(void)
{
  //TODO manage sequences, there are many packets to receive across this kind of calls

  return true;
}

COLD bool TcpHandler::send_logout(void)
{
  static const char *buffer = reinterpret_cast<const char *>(&logout_request);
  constexpr uint32_t total_size = sizeof(buffer);
  static uint32_t sent_bytes = 0;

  sent_bytes += utils::try_tcp_send(sock_fd, buffer + sent_bytes, total_size - sent_bytes);
  last_incoming = std::chrono::steady_clock::now();

  return (sent_bytes == total_size);
}
bool TcpHandler::send_hearbeat(void)
{
  static const char *buffer = reinterpret_cast<const char *>(&user_heartbeat);
  constexpr uint32_t total_size = sizeof(buffer);
  static uint32_t sent_bytes = 0;

  sent_bytes += utils::try_tcp_send(sock_fd, buffer + sent_bytes, total_size - sent_bytes);
  last_outgoing = std::chrono::steady_clock::now();

  return (sent_bytes == total_size);
}