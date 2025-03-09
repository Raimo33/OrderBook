#include <arpa/inet.h>
#include <netinet/tcp.h>
#include <unistd.h>

#include "TcpHandler.hpp"

TcpHandler::TcpHandler(struct sockaddr_in& addr, OrderBook& order_book) :
  state(DISCONNECTED),
  order_book(order_book)
{
  fd = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0);

  constexpr int enable = 1;
  setsockopt(fd, IPPROTO_TCP, TCP_FASTOPEN, &enable, sizeof(enable));
  setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, &enable, sizeof(enable));

  connect(fd, reinterpret_cast<struct sockaddr *>(&addr), sizeof(addr));
}

TcpHandler::~TcpHandler(void)
{
  close(fd);
}

void TcpHandler::request_snapshot(void)
{
  //TODO state machine, blocking because the separation is already done

  //exit thread at the end
}