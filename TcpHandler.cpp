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
      state += connect(sock_fd, reinterpret_cast<const sockaddr *>(&glimpse_address), sizeof(glimpse_address));
      break;
    case 1:
      state += send_login();
      break;
    case 2:
      state += recv_login();
      break;
    case 3:
      state += recv_snapshot();
      break;
    case 4:
      state += send_logout();
      break;
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
  constexpr uint32_t total_size = sizeof(login_request.length) + sizeof(login_request.body.type) + sizeof(login_request.body.login_request);
  static uint32_t sent_bytes = 0;

  sent_bytes += utils::try_tcp_send(sock_fd, buffer + sent_bytes, total_size - sent_bytes);
  if (sent_bytes < total_size)
    return false;

  last_outgoing = std::chrono::steady_clock::now();
  return true;
}

COLD bool TcpHandler::recv_login(void)
{
  static uint8_t state = 0;
  static SoupBinTCPPacket packet;
  static uint16_t bytes_read = 0;

  switch (state)
  {
    case 0:
      state += read_header(packet);
      break;
    case 1:
      switch (packet.body.type)
      {
        case 'H':
          last_incoming = std::chrono::steady_clock::now();
          return false;
        case 'J':
          utils::throw_exception("Login rejected");
        case 'A':
          constexpr uint8_t body_size = sizeof(SoupBinTCPPacket::Body::LoginAcceptance);
          bytes_read += utils::try_recv(sock_fd, reinterpret_cast<char *>(&packet.body.login_acceptance) + bytes_read, body_size - bytes_read);
          sequence_number = utils::atoul(packet.body.login_acceptance.sequence_number);
          return (bytes_read >= body_size);
        default:
          utils::throw_exception("Invalid packet type");
      }
  }
}

bool TcpHandler::recv_snapshot(void)
{
  static uint8_t state = 0;
  static uint16_t bytes_read = 0;
  static SoupBinTCPPacket packet;
  static std::vector<char> buffer(4096);

  switch (state)
  {
    case 0:
      state += read_header(packet);
      break;
    case 1:
      switch (packet.body.type)
      {
        case 'H':
          last_incoming = std::chrono::steady_clock::now();
          return false;
        case 'S':
          const uint32_t payload_size = packet.length - sizeof(packet.body.type);
          buffer.reserve(payload_size);
          bytes_read += utils::try_recv(sock_fd, buffer.data() + bytes_read, payload_size - bytes_read);
          if (bytes_read < payload_size)
            return false;
          bytes_read = 0;
          state = 0;
          buffer.clear();
          return process_message_blocks(buffer);
        default:
          utils::throw_exception("Invalid packet type");
      }
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

bool TcpHandler::read_header(SoupBinTCPPacket &packet) const
{
  constexpr uint16_t header_size = sizeof(packet.length) + sizeof(packet.body.type);
  static uint16_t bytes_read = 0;

  bytes_read += utils::try_recv(sock_fd, reinterpret_cast<char *>(&packet) + bytes_read, header_size - bytes_read);

  const bool complete = (bytes_read >= header_size);
  bytes_read *= !complete;
  return complete;
}

bool TcpHandler::process_message_blocks(const std::vector<char> &buffer)
{


  //returns truee if snapshot completion package found, false otherwise
}