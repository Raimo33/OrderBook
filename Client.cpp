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
}

void Client::create_handlers(void)
{
  tcp_handler = std::make_unique<TcpHandler>(config.client_conf, config.server_confs[server_idx], order_book);
  udp_handler = std::make_unique<UdpHandler>(config.client_conf, config.server_confs[server_idx], order_book);

  add_to_epoll(tcp_handler->get_fd());
  add_to_epoll(udp_handler->get_fd());
}

Client::~Client(void)
{
  close(epoll_fd);
}

COLD void Client::add_to_epoll(const int fd)
{
  constexpr uint32_t event_mask = EPOLLIN | EPOLLOUT | EPOLLERR | EPOLLHUP | EPOLLRDHUP | EPOLLET;
  epoll_event event = {.events = event_mask, .data = {.fd = fd}};
  epoll_ctl(epoll_fd, EPOLL_CTL_ADD, fd, &event);
}

COLD void Client::remove_from_epoll(const int fd)
{
  epoll_ctl(epoll_fd, EPOLL_CTL_DEL, fd, nullptr);
}

void Client::run(void)
{
  const int tcp_fd = tcp_handler->get_fd();
  // const int udp_fd = udp_handler->get_fd();
  
  {
    while (tcp_handler->get_state() != TcpHandler::LOGGED_OUT)
    {
      epoll_event events[2];
      uint8_t n = epoll_wait(epoll_fd, events, 2, -1);

      while (n--)
      {
        const epoll_event event = events[n];
        const uint32_t mask = event.events;

        if (LIKELY(event.data.fd == tcp_fd))
          tcp_handler->request_snapshot(mask);
        else
          udp_handler->accumulate_updates(mask);
      }
    }
  }

  remove_from_epoll(tcp_fd);

  {
    while (true)
    {
      epoll_event event;
      epoll_wait(epoll_fd, &event, 1, -1);

      udp_handler->process_updates(event.events);
      //TODO switch server if error
      // engine.print_orderbook();
    }
  }
}

void Client::switch_server(void)
{
  remove_from_epoll(tcp_handler->get_fd());
  remove_from_epoll(udp_handler->get_fd());

  server_idx ^= 1;

  create_handlers();
}