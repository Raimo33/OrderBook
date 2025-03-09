#include <arpa/inet.h>
#include <netinet/tcp.h>
#include <unistd.h>
#include <stdexcept>
#include <chrono>

#include "TcpHandler.hpp"

using namespace std::chrono_literals;

TcpHandler::TcpHandler(OrderBook& order_book) :
  state(DISCONNECTED),
  order_book(order_book)
{
  //TODO parse ips and port

  fd = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0);

  constexpr int enable = 1;
  setsockopt(fd, IPPROTO_TCP, TCP_FASTOPEN, &enable, sizeof(enable));
  setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, &enable, sizeof(enable));

  connect(fd, reinterpret_cast<struct sockaddr *>(&glimpse_address), sizeof(glimpse_address));
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