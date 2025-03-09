#include <arpa/inet.h>
#include <unistd.h>

#include "UdpHandler.hpp"

UdpHandler::UdpHandler(struct sockaddr_in& addr, OrderBook& order_book) :
  order_book(order_book)
{
  mreq.imr_multiaddr.s_addr = addr.sin_addr.s_addr;
  mreq.imr_interface.s_addr = htonl(INADDR_ANY);

  fd = socket(AF_INET, SOCK_DGRAM | SOCK_NONBLOCK, 0);

  constexpr int enable = 1;
  setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(enable));
  setsockopt(fd, SOL_SOCKET, SO_REUSEPORT, &enable, sizeof(enable));

  constexpr uint32_t buffer_size = 2 * 1024 * 1024;
  setsockopt(fd, SOL_SOCKET, SO_RCVBUF, &buffer_size, sizeof(buffer_size));

  setsockopt(fd, IPPROTO_IP, IP_ADD_MEMBERSHIP, &mreq, sizeof(mreq));

  bind(fd, reinterpret_cast<struct sockaddr *>(&addr), sizeof(addr));
  connect(fd, reinterpret_cast<struct sockaddr *>(&addr), sizeof(addr));
}

UdpHandler::~UdpHandler(void)
{
  setsockopt(fd, IPPROTO_IP, IP_DROP_MEMBERSHIP, &mreq, sizeof(mreq));
  close(fd);
}

void UdpHandler::accumulate_updates(void)
{
  while (true)
  {
    //read packet
    //put in queue (by sorting by sequence number)
  }
}

void UdpHandler::process_updates(void)
{
  while (true)
  {
    //read packet
    //put in queue (by sorting by sequence number)
    //empty the queue as long as you extract packets in order
  }
}
