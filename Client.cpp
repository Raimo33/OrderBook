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

Client::Client(const std::string_view ip_str, const uint16_t port) :
  tcp_sequence(0),
  udp_sequence(0)
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

void Client::init_udp_socket(void)
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

void Client::init_tcp_socket(void)
{
  tcp_fd = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0);

  constexpr int enable = 1;
  setsockopt(tcp_fd, IPPROTO_TCP, TCP_FASTOPEN, &enable, sizeof(enable));
  setsockopt(tcp_fd, IPPROTO_TCP, TCP_NODELAY, &enable, sizeof(enable));

  connect(tcp_fd, (struct sockaddr *)&addr, sizeof(addr));
}

void Client::init_epoll(void)
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
  struct epoll_event events[2];
  void (Client::*handlers[2])(uint32_t) = {&Client::handle_tcp_event, &Client::handle_udp_event};

  while (true)
  {
    uint8_t nfds = epoll_wait(epoll_fd, events, 2, -1);

    for (uint8_t i = 0; i < nfds; i++)
    {
      const struct epoll_event ev = events[i];
      if (ev.events & (EPOLLERR | EPOLLHUP | EPOLLRDHUP))
        handle_error(ev.data.fd, ev.events);
      
      const uint8_t idx = (ev.data.fd == udp_fd);
      (this->*handlers[idx])(ev.events);
    }
  }
}

inline void Client::handle_udp_event(const uint32_t ev)
{
  //TODO maybe dont even care about EPOLLOUT or EPOLLIN.
}

void Client::handle_tcp_event(const uint32_t ev)
{
  switch (tcp_sequence)
  {
    case 0:
      sequence += send_login_request(ev);
      break;
    case 1:
      if (UNLIKELY(!recv_login_response(ev)))
        return;
      sequence++;
    case 2:
      if (UNLIKELY(!recv_snapshot(ev)))
        return;
      sequence++;
    case 3:
      if (UNLIKELY(!send_logout_request(ev)))
        return;
      sequence++;
    case 4:
      sequence = 0;
    default:
      UNREACHABLE;
  }
}

void Client::send_login_request(const uint32_t ev)
{
  //TODO move semantics to move ownership of the string to the buffer
}

NEVER_INLINE void handle_error(const int fd, const uint32_t ev)
{
  std::ostringstream oss;
  oss << "Error on file descriptor " << fd << " with event " << ev;
  throw std::runtime_error(oss.str());
}

void Client::free_udp_socket(void)
{
  setsockopt(udp_fd, IPPROTO_IP, IP_DROP_MEMBERSHIP, &mreq, sizeof(mreq));
  close(udp_fd);
}

void Client::free_tcp_socket(void)
{
  close(tcp_fd);
}

void Client::free_epoll(void)
{
  close(epoll_fd);
}