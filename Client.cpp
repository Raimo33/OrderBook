/*================================================================================

File: Client.cpp                                                                
Creator: Claudio Raimondi                                                       
Email: claudio.raimondi@pm.me                                                   

created at: 2025-03-08 15:48:16                                                 
last edited: 2025-03-08 21:24:05                                                

================================================================================*/

#include <arpa/inet.h>
#include <sys/epoll.h>
#include <unistd.h>

#include "Client.hpp"
#include "Config.hpp"
#include "macros.hpp"

Client::Client(const Config &config) :
  config(config),
  server_idx(0),
  epoll_fd(epoll_create1(0))
{
  create_handlers();
  bind_epoll();
}

void Client::create_handlers(void)
{
  tcp_handler = std::make_unique<TcpHandler>(config.client_conf, config.server_confs[server_idx], order_book);
  udp_handler = std::make_unique<UdpHandler>(config.client_conf, config.server_confs[server_idx], order_book);
}

void Client::bind_epoll(void) const noexcept
{
  constexpr uint32_t network_events_mask = EPOLLIN | EPOLLERR | EPOLLHUP | EPOLLRDHUP | EPOLLET;
  constexpr uint32_t timer_events_mask = EPOLLIN | EPOLLET;

  const int tcp_sock_fd = tcp_handler->get_sock_fd();
  const int udp_sock_fd = udp_handler->get_sock_fd();
  const int tcp_timer_fd = tcp_handler->get_timer_fd();
  const int udp_timer_fd = udp_handler->get_timer_fd();

  epoll_event events[4] = {
    { .events = network_events_mask, .data = {.fd = tcp_sock_fd} },
    { .events = network_events_mask, .data = {.fd = udp_sock_fd} },
    { .events = timer_events_mask, .data = {.fd = tcp_timer_fd} },
    { .events = timer_events_mask, .data = {.fd = udp_timer_fd} }
  };

  epoll_ctl(epoll_fd, EPOLL_CTL_ADD, tcp_sock_fd, &events[0]);
  epoll_ctl(epoll_fd, EPOLL_CTL_ADD, udp_sock_fd, &events[1]);
  epoll_ctl(epoll_fd, EPOLL_CTL_ADD, tcp_timer_fd, &events[2]);
  epoll_ctl(epoll_fd, EPOLL_CTL_ADD, udp_timer_fd, &events[3]);
}

void Client::unbind_epoll(void) const noexcept
{
  epoll_ctl(epoll_fd, EPOLL_CTL_DEL, tcp_handler->get_sock_fd(), nullptr);
  epoll_ctl(epoll_fd, EPOLL_CTL_DEL, udp_handler->get_sock_fd(), nullptr);
  epoll_ctl(epoll_fd, EPOLL_CTL_DEL, tcp_handler->get_timer_fd(), nullptr);
  epoll_ctl(epoll_fd, EPOLL_CTL_DEL, udp_handler->get_timer_fd(), nullptr);
}

Client::~Client(void)
{
  unbind_epoll();
  close(epoll_fd);
}

void Client::run(void)
{
  const int tcp_sock_fd = tcp_handler->get_sock_fd();
  const int udp_sock_fd = udp_handler->get_sock_fd();
  const int tcp_timer_fd = tcp_handler->get_timer_fd();
  const int udp_timer_fd = udp_handler->get_timer_fd();
  
  {
    while (tcp_handler->get_state() != TcpHandler::LOGGED_OUT)
    {
      std::array<epoll_event, 4> events;
      uint8_t n = epoll_wait(epoll_fd, events.data(), events.size(), -1);

      while (n--)
      {
        const epoll_event event = events[n];
        const uint32_t mask = event.events;

        //TODO branchless
        if (LIKELY(event.data.fd == tcp_sock_fd))
          tcp_handler->request_snapshot(mask);
        else if (event.data.fd == udp_sock_fd)
          udp_handler->accumulate_updates(mask);
        else if (event.data.fd == tcp_timer_fd)
          tcp_handler->handle_heartbeat_timeout(mask);
        else if (event.data.fd == udp_timer_fd)
          udp_handler->handle_heartbeat_timeout(mask);
      }
    }
  }

  epoll_ctl(epoll_fd, EPOLL_CTL_DEL, tcp_sock_fd, nullptr);
  epoll_ctl(epoll_fd, EPOLL_CTL_DEL, tcp_timer_fd, nullptr);
  tcp_handler = nullptr;

  while (true)
  {
    std::array<epoll_event, 2> events;
    uint8_t n = epoll_wait(epoll_fd, events.data(), events.size(), -1);

    while (n--)
    {
      const epoll_event event = events[n];
      const uint32_t mask = event.events;

      if (event.data.fd == udp_sock_fd)
        udp_handler->process_updates(mask);
      else if (event.data.fd == udp_timer_fd)
        udp_handler->handle_heartbeat_timeout(mask);
    }

    // engine.print_orderbook();
  }

}

void Client::switch_server(void)
{
  unbind_epoll();

  server_idx ^= 1;

  create_handlers();
  bind_epoll();
}