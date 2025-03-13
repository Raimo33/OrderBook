#include <arpa/inet.h>
#include <unistd.h>
#include <sys/epoll.h>
#include <sys/timerfd.h>

#include "UdpHandler.hpp"
#include "Config.hpp"
#include "UdpPackets.hpp"
#include "utils.hpp"
#include "macros.hpp"

using namespace std::chrono_literals;

UdpHandler::UdpHandler(const ClientConfig &client_conf, const ServerConfig &server_conf, OrderBook &order_book) :
  order_book(&order_book),
  multicast_address(utils::create_address(server_conf.multicast_endpoint.ip, server_conf.multicast_endpoint.port)),
  rewind_address(utils::create_address(server_conf.rewind_endpoint.ip, server_conf.rewind_endpoint.port)),
  mreq(create_mreq(client_conf.bind_address)),
  sock_fd(create_socket()),
  timer_fd(utils::create_timer(50ms)),
  last_incoming(std::chrono::steady_clock::now())
{
  sockaddr_in bind_addr;
  bind_addr.sin_family = AF_INET;
  bind_addr.sin_port = htons(client_conf.udp_port);
  inet_pton(AF_INET, client_conf.bind_address.c_str(), &bind_addr.sin_addr);

  bind(sock_fd, reinterpret_cast<const sockaddr *>(&bind_addr), sizeof(bind_addr));
}

const ip_mreq UdpHandler::create_mreq(const std::string_view bind_address) const noexcept
{
  return {
    .imr_multiaddr = multicast_address.sin_addr,
    .imr_interface = inet_addr(bind_address.data())
  };
}

int UdpHandler::create_socket(void) const noexcept
{
  const int sock_fd = socket(AF_INET, SOCK_DGRAM | SOCK_NONBLOCK, 0);

  constexpr int enable = 1;
  setsockopt(sock_fd, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(enable));
  setsockopt(sock_fd, IPPROTO_IP, IP_ADD_MEMBERSHIP, &mreq, sizeof(mreq));

  return sock_fd;
}

UdpHandler::~UdpHandler(void)
{
  close(sock_fd);
}

UdpHandler &UdpHandler::operator=(const UdpHandler &other)
{
   if (this == &other)
     return *this;

  order_book = other.order_book;
  multicast_address = other.multicast_address;
  rewind_address = other.rewind_address;
  mreq = other.mreq;
  sock_fd = other.sock_fd;
  last_incoming = other.last_incoming;

  return *this;
}

void UdpHandler::accumulate_updates(const uint32_t event_mask)
{
  if (event_mask & (EPOLLERR | EPOLLHUP | EPOLLRDHUP))
    utils::throw_exception("Error in udp socket");

  //TODO state machine, use connect() to bind to the multicast address because i use normal send()
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
    //put orders in queue (by sorting by sequence number)
    //empty the queue as long as you extract packets in order
  }
}

void UdpHandler::handle_heartbeat_timeout(UNUSED const uint32_t event_mask)
{
  const auto now = std::chrono::steady_clock::now();

  if (now - last_incoming > 50ms)
    utils::throw_exception("Server timeout");
}