#include <arpa/inet.h>
#include <netinet/tcp.h>
#include <unistd.h>
#include <stdexcept>
#include <chrono>

#include "TcpHandler.hpp"
#include "Config.hpp"

using namespace std::chrono_literals;

TcpHandler::TcpHandler(const Config& config, OrderBook& order_book) :
  state(DISCONNECTED),
  order_book(order_book),
  last_outgoing(std::chrono::steady_clock::now()),
  last_incoming(std::chrono::steady_clock::now()),
{

  for (auto& glimpse_address : glimpse_addresses)
  {
    glimpse_address.sin_family = AF_INET;
    glimpse_address.sin_port = htons(config.glimpse_endpoints[0].port);
    inet_pton(AF_INET, config.glimpse_endpoints[0].ip.c_str(), &glimpse_address.sin_addr);
  }

  fd = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0);

  constexpr int enable = 1;
  setsockopt(fd, IPPROTO_TCP, TCP_FASTOPEN, &enable, sizeof(enable));
  setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, &enable, sizeof(enable));

  struct sockaddr_in bind_addr;
  bind_addr.sin_family = AF_INET;
  bind_addr.sin_port = htons(config.tcp_port);
  inet_pton(AF_INET, config.bind_address.c_str(), &bind_addr.sin_addr);

  bind(fd, reinterpret_cast<struct sockaddr *>(&bind_addr), sizeof(bind_addr));
  state = CONNECTED;
}

TcpHandler::~TcpHandler(void)
{
  close(fd);
}

void TcpHandler::request_snapshot(void)
{
  switch (state)
  {
    case DISCONNECTED:
      state += connect(fd, reinterpret_cast<struct sockaddr *>(&glimpse_addresses[0]), sizeof(glimpse_addresses[0]));
      break;
    case CONNECTED:
      state += send_login();
      break;
    case LOGIN_SENT:
      state += recv_login(); //if they receive a heartbeat they just update the last_incoming time
      break;
    case LOGIN_RECEIVED:
      state += recv_snapshot(); //if they receive a heartbeat they just update the last_incoming time
      break;
    case SNAPSHOT_RECEIVED:
      state += send_logout();
      break;
    case LOGGED_OUT:
      break;
  }

  auto now = std::chrono::steady_clock::now();
  if (last_outgoing + 1s >= now)
    send_hearbeat();
  if (last_incoming + 50ms >= now)
    throw std::runtime_error("Server timeout");
}

inline TcpHandler::State TcpHandler::get_state(void) const { return state; }

bool TcpHandler::send_login(void)
{
  
  return true;
}