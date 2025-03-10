#include <arpa/inet.h>
#include <unistd.h>

#include "UdpHandler.hpp"
#include "Config.hpp"
#include "utils.hpp"
#include "macros.hpp"

UdpHandler::UdpHandler(const ClientConfig &client_conf, const ServerConfig &server_conf, OrderBook &order_book) :
  order_book(&order_book),
  last_received(std::chrono::steady_clock::now())
{
  multicast_address.sin_family = AF_INET;
  multicast_address.sin_port = htons(server_conf.multicast_endpoint.port);
  inet_pton(AF_INET, server_conf.multicast_endpoint.ip.c_str(), &multicast_address.sin_addr);

  rewind_address.sin_family = AF_INET;
  rewind_address.sin_port = htons(server_conf.rewind_endpoint.port);
  inet_pton(AF_INET, server_conf.rewind_endpoint.ip.c_str(), &rewind_address.sin_addr);

  const int fd = socket(AF_INET, SOCK_DGRAM | SOCK_NONBLOCK, 0);

  constexpr int enable = 1;
  setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(enable));

  sockaddr_in bind_addr;
  bind_addr.sin_family = AF_INET;
  bind_addr.sin_port = htons(client_conf.udp_port);
  inet_pton(AF_INET, client_conf.bind_address.c_str(), &bind_addr.sin_addr);

  mreq.imr_multiaddr = multicast_address.sin_addr;
  inet_pton(AF_INET, client_conf.bind_address.c_str(), &mreq.imr_interface);

  setsockopt(fd, IPPROTO_IP, IP_ADD_MEMBERSHIP, &mreq, sizeof(mreq));
  bind(fd, (sockaddr*)&bind_addr, sizeof(bind_addr));
}

UdpHandler::~UdpHandler(void)
{
  close(fd);
}

UdpHandler &UdpHandler::operator=(UdpHandler &other)
{
   if (this == &other)
     return *this;

  order_book = other.order_book;
  multicast_address = other.multicast_address;
  rewind_address = other.rewind_address;
  mreq = other.mreq;
  fd = other.fd;
  last_received = other.last_received;

  return *this;
}

inline int UdpHandler::get_fd(void) const { return fd; }

void UdpHandler::accumulate_updates(const uint32_t event_mask)
{
  if (event_mask & (EPOLLERR | EPOLLHUP | EPOLLRDHUP))
    utils::throw_exception("Error in udp socket");

  {
    //read packet
    //put in queue (by sorting by sequence number)
  }
}

void UdpHandler::process_updates(const uint32_t event_mask)
{
  if (event_mask & (EPOLLERR | EPOLLHUP | EPOLLRDHUP))
    utils::throw_exception("Error in udp socket");

  {
    //read packet
    //put in queue (by sorting by sequence number)
    //empty the queue as long as you extract packets in order
  }
}