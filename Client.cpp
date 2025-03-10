/*================================================================================

File: Client.cpp                                                                
Creator: Claudio Raimondi                                                       
Email: claudio.raimondi@pm.me                                                   

created at: 2025-03-08 15:48:16                                                 
last edited: 2025-03-08 21:24:05                                                

================================================================================*/

#include <arpa/inet.h>
#include <sys/epoll.h>

#include "Client.hpp"
#include "Config.hpp"
#include "macros.hpp"

Client::Client(const Config &config) :
  config(config),
  server_idx(0),
  tcp_handler(config.client_conf, config.server_confs[server_idx], order_book),
  udp_handler(config.client_conf, config.server_confs[server_idx], order_book),
  epoll_fd(epoll_create1(0))
{
  init_epoll(void);
}

Client::~Client(void) {}

COLD void Client::init_epoll(void)
{
  const int tcp_fd = tcp_handler.get_fd();
  const int udp_fd = udp_handler.get_fd();

  constexpr uint32_t event_mask = EPOLLIN | EPOLLOUT | EPOLLERR | EPOLLHUP | EPOLLRDHUP | EPOLLET;
  epoll_event events[2] = {
    {.events = event_mask, .data.fd = tcp_fd},
    {.events = event_mask, .data.fd = udp_fd}
  };
  epoll_ctl(epoll_fd, EPOLL_CTL_ADD, tcp_fd, &events[0]);
  epoll_ctl(epoll_fd, EPOLL_CTL_ADD, udp_fd, &events[1]);
}

void Client::run(void)
{
  const int tcp_fd = tcp_handler.get_fd();
  const int udp_fd = udp_handler.get_fd();

  {
    while (tcp_handler.get_state() != TcpHandler::LOGGED_OUT)
    {
      epoll_event events[2];
      uint8_t n = epoll_wait(epoll_fd, events, 2, -1);

      while (n--)
      {
        const epoll_event event = events[n];
        const uint32_t mask = event.events;

        if (event.data.fd == tcp_fd)
          tcp_handler.request_snapshot(mask);
        else
          udp_handler.accumulate_updates(mask);
      }
    }
  }

  epoll_ctl(epoll_fd, EPOLL_CTL_DEL, tcp_fd, nullptr);

  {
    while (true)
    {
      epoll_event event;
      uint8_t n = epoll_wait(epoll_fd, &event, 1, -1);

      udp_handler.process_updates(event.events);
      //switch server if error
      engine.print_orderbook();
    }
  }
}

void Client::switch_server(void)
{
  server_idx ^= 1;

  tcp_handler = TcpHandler(config.client_conf, config.server_confs[server_idx], order_book);
  udp_handler = UdpHandler(config.client_conf, config.server_confs[server_idx], order_book);
}