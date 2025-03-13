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
#include "Packets.hpp"
#include "utils.hpp"
#include "macros.hpp"

using namespace std::chrono_literals;

TcpHandler::TcpHandler(const ClientConfig &client_conf, const ServerConfig &server_conf, OrderBook &order_book) :
  state(0),
  sequence_number(0),
  order_book(&order_book),
  glimpse_address(utils::create_address(server_conf.glimpse_endpoint.ip, server_conf.glimpse_endpoint.port)),
  sock_fd(create_socket()),
  timer_fd(utils::create_timer(50ms)),
  login_request(create_login_request(client_conf)),
  last_outgoing(std::chrono::steady_clock::now()),
  last_incoming(std::chrono::steady_clock::now())
{
  sockaddr_in bind_addr;
  bind_addr.sin_family = AF_INET;
  bind_addr.sin_port = htons(client_conf.tcp_port);
  inet_pton(AF_INET, client_conf.bind_address.c_str(), &bind_addr.sin_addr);

  bind(sock_fd, reinterpret_cast<sockaddr *>(&bind_addr), sizeof(bind_addr));
}

const SoupBinTCPPacket TcpHandler::create_login_request(const ClientConfig &client_conf) const noexcept
{
  SoupBinTCPPacket packet{};

  packet.length = sizeof(packet.body.type) + sizeof(SoupBinTCPPacket::Body::LoginRequest);
  packet.body.type = 'L';

  auto &login = packet.body.login_request;

  std::memcpy(login.username, client_conf.username.c_str(), sizeof(login.username));
  std::memcpy(login.password, client_conf.password.c_str(), sizeof(login.password));
  std::memset(login.requested_session, ' ', sizeof(login.requested_session));
  login.requested_sequence_number[0] = '1';

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
      if (!connect(sock_fd, reinterpret_cast<const sockaddr *>(&glimpse_address), sizeof(glimpse_address)))
        break;
      state++;
    case 1:
      if (!send_login())
        break;
      state++;
    case 2:
      if (!recv_login())
        break;
      state++;
    case 3:
      if (!recv_snapshot())
        break;
      state++;
    case 4:
      if (!send_logout())
        break;
      state++;
    default:
      return;
  }
}

void TcpHandler::handle_heartbeat_timeout(UNUSED const uint32_t event_mask)
{
  const auto now = std::chrono::steady_clock::now();

  uint64_t _;
  read(timer_fd, &_, sizeof(_));

  if (UNLIKELY(now - last_incoming > 50ms))
    utils::throw_exception("Server timeout");
  if (UNLIKELY(now - last_outgoing > 1s))
    send_hearbeat();
}

COLD bool TcpHandler::send_login(void)
{
  static const char *buffer = reinterpret_cast<const char *>(&login_request);
  constexpr uint32_t total_size = soupbin_tcp_packet_size<SoupBinTCPPacket::Body::LoginRequest>;
  static uint32_t sent_bytes = 0;

  sent_bytes += utils::try_tcp_send(sock_fd, buffer + sent_bytes, total_size - sent_bytes);
  if (sent_bytes < total_size)
    return false;

  last_outgoing = std::chrono::steady_clock::now();
  return true;
}

COLD bool TcpHandler::recv_login(void)
{
  static SoupBinTCPPacket packet;
  static char *buffer = reinterpret_cast<char *>(&packet);
  constexpr uint32_t total_size = soupbin_tcp_packet_size<SoupBinTCPPacket::Body::LoginAcceptance>;
  constexpr uint8_t minimum_size = sizeof(packet.length) + sizeof(packet.body.type);
  static uint32_t received_bytes = 0;

  received_bytes += utils::try_recv(sock_fd, buffer + received_bytes, total_size - received_bytes);
  if (received_bytes < minimum_size)
    return false;

  switch (packet.body.type)
  {
    case 'H':
      constexpr uint8_t heartbeat_size = minimum_size;
      std::memmove(buffer, buffer + heartbeat_size, total_size - heartbeat_size); //TODO use ring buffer instead
      received_bytes -= heartbeat_size;
      last_incoming = std::chrono::steady_clock::now();
      return false;
    case 'A':
      if (received_bytes < total_size)
        return false;
      sequence_number = utils::atoul(packet.body.login_acceptance.sequence_number);
      last_incoming = std::chrono::steady_clock::now();
      return true;
    case 'J':
      utils::throw_exception("Login rejected");
    default:
      utils::throw_exception("Unexpected packet during login");
  }
}

bool TcpHandler::recv_snapshot(void)
{
  static char buffer[1024];
  static SoupBinTCPPacket& packet = *reinterpret_cast<SoupBinTCPPacket *>(buffer);
  static uint32_t received_bytes = 0;
  constexpr uint8_t minimum_size = sizeof(packet.length) + sizeof(packet.body.type);

  received_bytes += utils::try_recv(sock_fd, buffer + received_bytes, sizeof(buffer) - received_bytes);
  if (received_bytes < minimum_size)
    return false;

  switch (packet.body.type)
  {
    case 'H':
      constexpr uint8_t heartbeat_size = minimum_size;
      std::memmove(buffer, buffer + heartbeat_size, sizeof(buffer) - heartbeat_size); //TODO use ring buffer instead
      last_incoming = std::chrono::steady_clock::now();
      return false;
    case 'S':
      //return false if the data received is smaller than the packet.length
      //process all the message blocks for packet.length bytes
      //increase sequence number
      last_incoming = std::chrono::steady_clock::now();
      //remove questo Data Packet dal buffer
      return //true se e' arrivato il message data di completion, false altrimenti
    default:
      utils::throw_exception("Unexpected packet during snapshot");
  }

}

COLD bool TcpHandler::send_logout(void)
{
  constexpr SoupBinTCPPacket logout_request = {
    .length = 1,
    .body.type = 'O'
  };
  static const char *buffer = reinterpret_cast<const char *>(&logout_request);
  constexpr uint32_t total_size = sizeof(logout_request.length) + sizeof(logout_request.body.type);
  static uint32_t sent_bytes = 0;

  sent_bytes += utils::try_tcp_send(sock_fd, buffer + sent_bytes, total_size - sent_bytes);
  if (sent_bytes < total_size)
    return false;

  last_outgoing = std::chrono::steady_clock::now();
  return true;
}

//TODO may not send everything in one go
bool TcpHandler::send_hearbeat(void)
{
  constexpr SoupBinTCPPacket user_heartbeat = {
    .length = 1,
    .body.type = 'H'
  };
  static const char *buffer = reinterpret_cast<const char *>(&user_heartbeat);
  constexpr uint32_t total_size = sizeof(user_heartbeat.length) + sizeof(user_heartbeat.body.type);
  static uint32_t sent_bytes = 0;

  sent_bytes += utils::try_tcp_send(sock_fd, buffer + sent_bytes, total_size - sent_bytes);
  if (sent_bytes < total_size)
    return false;

  last_outgoing = std::chrono::steady_clock::now();
  return true;
}