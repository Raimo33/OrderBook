#include <arpa/inet.h>
#include <unistd.h>

#include "UdpHandler.hpp"
#include "Config.hpp"

UdpHandler::UdpHandler(const Config& config, OrderBook& order_book) :
  order_book(order_book),
  last_received(std::chrono::steady_clock::now())
{

  for (auto& multicast_address : multicast_addresses)
  {
    multicast_address.sin_family = AF_INET;
    multicast_address.sin_port = htons(config.multicast_endpoints[0].port);
    inet_pton(AF_INET, config.multicast_endpoints[0].ip.c_str(), &multicast_address.sin_addr);

    //TODO mreqs
  }

  for (auto& rewind_address : rewind_addresses)
  {
    rewind_address.sin_family = AF_INET;
    rewind_address.sin_port = htons(config.rewind_endpoints[0].port);
    inet_pton(AF_INET, config.rewind_endpoints[0].ip.c_str(), &rewind_address.sin_addr);
  }

  const int fd = socket(AF_INET, SOCK_DGRAM | SOCK_NONBLOCK, 0);

  constexpr int enable = 1;
  setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(enable));

  sockaddr_in bind_addr;
  bind_addr.sin_family = AF_INET;
  bind_addr.sin_port = htons(config.udp_port);
  inet_pton(AF_INET, config.bind_address.c_str(), &bind_addr.sin_addr);

  bind(fd, (sockaddr*)&bind_addr, sizeof(bind_addr));
  //TODO join multicast group
}

UdpHandler::~UdpHandler(void)
{
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