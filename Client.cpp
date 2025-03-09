/*================================================================================

File: Client.cpp                                                                
Creator: Claudio Raimondi                                                       
Email: claudio.raimondi@pm.me                                                   

created at: 2025-03-08 15:48:16                                                 
last edited: 2025-03-08 21:24:05                                                

================================================================================*/

#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <cstring>
#include <unistd.h>
#include <sys/epoll.h>
#include <stdexcept>

#include "Client.hpp"
#include "macros.hpp"

//TODO bind to specific static IP instead of INADDR_ANY

Client::Client(const std::string_view ip_str, const uint16_t port) :
  tcp_sequence(DISCONNECTED)
{
  std::memset(&addr, 0, sizeof(addr));
  addr.sin_family = AF_INET;
  addr.sin_port = htons(port);
  addr.sin_addr.s_addr = htonl(INADDR_ANY);
  
  std::memset(&mreq, 0, sizeof(mreq));
  inet_pton(AF_INET, ip_str.data(), &mreq.imr_multiaddr.s_addr);
  mreq.imr_interface.s_addr = htonl(INADDR_ANY);

  init_udp_socket();
  init_tcp_socket();
  init_epoll();
}

Client::~Client(void)
{
  free_epoll();
  free_tcp_socket();
  free_udp_socket();
}

COLD void Client::init_udp_socket(void)
{
  udp_fd = socket(AF_INET, SOCK_DGRAM | SOCK_NONBLOCK, 0);

  constexpr int enable = 1;
  setsockopt(udp_fd, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(enable));
  setsockopt(udp_fd, SOL_SOCKET, SO_REUSEPORT, &enable, sizeof(enable));

  constexpr uint32_t buffer_size = 2 * 1024 * 1024;
  setsockopt(udp_fd, SOL_SOCKET, SO_RCVBUF, &buffer_size, sizeof(buffer_size));

  setsockopt(udp_fd, IPPROTO_IP, IP_ADD_MEMBERSHIP, &mreq, sizeof(mreq));

  bind(udp_fd, (struct sockaddr *)&addr, sizeof(addr));
  connect(udp_fd, (struct sockaddr *)&addr, sizeof(addr));
}

COLD void Client::init_tcp_socket(void)
{
  tcp_fd = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0);

  constexpr int enable = 1;
  setsockopt(tcp_fd, IPPROTO_TCP, TCP_FASTOPEN, &enable, sizeof(enable));
  setsockopt(tcp_fd, IPPROTO_TCP, TCP_NODELAY, &enable, sizeof(enable));

  connect(tcp_fd, (struct sockaddr *)&addr, sizeof(addr));
}

COLD void Client::init_epoll(void)
{
  epoll_fd = epoll_create1(0);

  constexpr uint32_t events = EPOLLIN | EPOLLOUT | EPOLLHUP | EPOLLRDHUP | EPOLLERR | EPOLLET;
  struct epoll_event tcp_event = {events, {.fd = tcp_fd}};
  struct epoll_event udp_event = {events, {.fd = udp_fd}};

  epoll_ctl(epoll_fd, EPOLL_CTL_ADD, tcp_fd, &tcp_event);
  epoll_ctl(epoll_fd, EPOLL_CTL_ADD, udp_fd, &udp_event);
}

void Client::run(void)
{
  fetch_snapshot();
  update_orderbook();
}

COLD void Client::fetch_snapshot(void)
{
  struct epoll_event events[2] = {0};

  while (tcp_sequence != LOGGED_OUT)
  {
    const int nfds = epoll_wait(epoll_fd, events, 2, -1);

    for (int i = 0; i < nfds; i++)
    {
      if (events[i].data.fd == tcp_fd)
        handle_tcp_event(events[i].events);
      else
        handle_udp_event(events[i].events);
    }
  }

  free_tcp_socket();
}

void Client::update_orderbook(void)
{
  struct epoll_event ev = {0};

  while (true)
  {
    epoll_wait(epoll_fd, &ev, 1, -1);
    handle_udp_event(ev.events);
  }
}

HOT inline void Client::handle_udp_event(const uint32_t ev)
{
  const bool error = ev & (EPOLLERR | EPOLLHUP | EPOLLRDHUP);
  if (UNLIKELY(error))
    throw std::runtime_error("UDP connection closed unexpectedly");

  //TODO almost always epollin, queue the packet
}

COLD void Client::handle_tcp_event(const uint32_t ev)
{
  const bool error = ev & (EPOLLERR | EPOLLHUP | EPOLLRDHUP);
  if (UNLIKELY(error & (tcp_sequence != DISCONNECTED)))
    throw std::runtime_error("TCP connection closed unexpectedly");

  switch (tcp_sequence)
  {
    case DISCONNECTED:
      if (LIKELY(send_login_request(ev)))
        tcp_sequence = LOGIN_SENT;
      return;
    case LOGIN_SENT:
      if (UNLIKELY(!recv_login_response(ev)))
        return;
      tcp_sequence = LOGIN_RECEIVED;
    case LOGIN_RECEIVED:
      if (UNLIKELY(!recv_snapshot(ev)))
        return;
      tcp_sequence = SNAPSHOT_RECEIVED;
    case SNAPSHOT_RECEIVED:
      if (UNLIKELY(!send_logout_request(ev)))
        return;
      tcp_sequence = LOGGED_OUT;
    default:
      return;
  }
}

COLD void Client::send_login_request(const uint32_t ev)
{
  //TODO move semantics to move ownership of the string to the buffer
}

void Client::free_udp_socket(void)
{
  setsockopt(udp_fd, IPPROTO_IP, IP_DROP_MEMBERSHIP, &mreq, sizeof(mreq));
  epoll_ctl(epoll_fd, EPOLL_CTL_DEL, udp_fd, nullptr);
  close(udp_fd);
}

void Client::free_tcp_socket(void)
{
  epoll_ctl(epoll_fd, EPOLL_CTL_DEL, tcp_fd, nullptr);
  close(tcp_fd);
}

void Client::free_epoll(void)
{
  close(epoll_fd);
}