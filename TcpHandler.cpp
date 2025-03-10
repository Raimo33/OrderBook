#include <arpa/inet.h>
#include <netinet/tcp.h>
#include <sys/epoll.h>
#include <unistd.h>
#include <stdexcept>
#include <chrono>

#include "TcpHandler.hpp"
#include "Config.hpp"
#include "utils.hpp"
#include "macros.hpp"

using namespace std::chrono_literals;

TcpHandler::TcpHandler(const ClientConfig &client_conf, const ServerConfig &server_conf, OrderBook &order_book) :
  state(DISCONNECTED),
  order_book(&order_book),
  last_outgoing(std::chrono::steady_clock::now()),
  last_incoming(std::chrono::steady_clock::now())
{
  glimpse_address.sin_family = AF_INET;
  glimpse_address.sin_port = htons(server_conf.glimpse_endpoint.port);
  inet_pton(AF_INET, server_conf.glimpse_endpoint.ip.c_str(), &glimpse_address.sin_addr);

  fd = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0);

  constexpr int enable = 1;
  setsockopt(fd, IPPROTO_TCP, TCP_FASTOPEN, &enable, sizeof(enable));
  setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, &enable, sizeof(enable));

  sockaddr_in bind_addr;
  bind_addr.sin_family = AF_INET;
  bind_addr.sin_port = htons(client_conf.tcp_port);
  inet_pton(AF_INET, client_conf.bind_address.c_str(), &bind_addr.sin_addr);

  bind(fd, reinterpret_cast<sockaddr *>(&bind_addr), sizeof(bind_addr));
}

TcpHandler::~TcpHandler(void)
{
  close(fd);
}

TcpHandler &TcpHandler::operator=(const TcpHandler &other)
{
  if (this == &other)
    return *this;

  state = other.state;
  fd = other.fd;
  glimpse_address = other.glimpse_address;
  order_book = other.order_book;
  last_outgoing = other.last_outgoing;
  last_incoming = other.last_incoming;
}

COLD void TcpHandler::request_snapshot(const uint32_t event_mask)
{
  if (event_mask & (EPOLLERR | EPOLLHUP | EPOLLRDHUP))
    utils::throw_exception("Error in tcp socket");

  switch (state)
  {
    case DISCONNECTED:
      state += connect(fd, reinterpret_cast<sockaddr *>(&glimpse_address), sizeof(glimpse_address));
      break;
    case CONNECTED:
      state += send_login(event_mask);
      break;
    case LOGIN_SENT:
      state += recv_login(event_mask); //if they receive a heartbeat they just update the last_incoming time
      break;
    case LOGIN_RECEIVED:
      state += recv_snapshot(event_mask); //if they receive a heartbeat they just update the last_incoming time
      break;
    case SNAPSHOT_RECEIVED:
      state += send_logout(event_mask);
      break;
    case LOGGED_OUT:
      break;
  }

  auto now = std::chrono::steady_clock::now();
  if (last_outgoing + 1s >= now)
    send_hearbeat(event_mask);
  if (last_incoming + 50ms >= now)
    utils::throw_exception("Server timeout");
}

inline TcpHandler::State TcpHandler::get_state(void) const { return state; }

inline int TcpHandler::get_fd(void) const { return fd; }

bool TcpHandler::send_login(const uint32_t event_mask)
{
  utils::assert(event_mask & EPOLLOUT, "Socket not writeable");


  return true;
}

COLD bool TcpHandler::recv_login(const uint32_t event_mask)
{
  utils::assert(event_mask & EPOLLIN, "Socket not readable");

  return true;
}

bool TcpHandler::recv_snapshot(const uint32_t event_mask)
{
  utils::assert(event_mask & EPOLLIN, "Socket not readable");

  return true;
}

COLD bool TcpHandler::send_logout(const uint32_t event_mask)
{
  utils::assert(event_mask & EPOLLOUT, "Socket not writeable");

  //TODO sends shutdown, does not close the socket
  return true;
}
bool TcpHandler::send_hearbeat(const uint32_t event_mask)
{
  utils::assert(event_mask & EPOLLOUT, "Socket not writeable");

  return true;
}