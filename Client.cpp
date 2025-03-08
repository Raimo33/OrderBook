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
  tcp_state(DISCONNECTED)
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
  tcp_state = CONNECTED;
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
  constexpr uint16_t buffer_size = 1500;
  alignas(64) char buffer[buffer_size];

  struct epoll_event events[2];
  void (Client::*handlers[2])(uint32_t, char*, uint16_t) = {&Client::handle_tcp_event, &Client::handle_udp_event};

  while (true)
  {
    int nfds = epoll_wait(epoll_fd, events, 2, -1);

    for (int i = 0; i < nfds; i++)
    {
      const struct epoll_event ev = events[i];
      
      const uint8_t idx = (ev.data.fd == udp_fd);
      (this->*handlers[idx])(ev.events, buffer, buffer_size);
    }
  }
}

inline void Client::handle_udp_event(const uint32_t ev, char *buffer, const uint16_t buffer_size)
{
  if (ev & (EPOLLERR | EPOLLHUP | EPOLLRDHUP))
  {
    handle_error(udp_fd, ev);
    return;
  }

  //TODO maybe dont even care about EPOLLOUT or EPOLLIN.
}

void Client::handle_tcp_event(const uint32_t ev, char *buffer, const uint16_t buffer_size)
{
  if (ev & (EPOLLERR | EPOLLHUP | EPOLLRDHUP))
  {
    handle_error(tcp_fd, ev);
    return;
  }

  switch (state)
  {
    case DISCONNECTED:
      throw std::runtime_error("TCP socket not initialized");
      break;
    case CONNECTED:
      send_login_request(ev);
      state = ORDERBOOK_REQUESTED;
      break;
    case ORDERBOOK_REQUESTED:
      receive_orderbook(ev, buffer, buffer_size);
      state = ORDERBOOK_RECEIVED;
      break;
    case ORDERBOOK_RECEIVED:
      send_logout_request(ev);
      state = DISCONNECTED;
      break;
    default:
      UNREACHABLE;
  }
}

void Client::free_udp_socket(void)
{
  setsockopt(udp_fd, IPPROTO_IP, IP_DROP_MEMBERSHIP, &mreq, sizeof(mreq));
  close(udp_fd);
}

NEVER_INLINE void handle_error(const int fd, const uint32_t ev)
{
  std::ostringstream oss;
  oss << "Error on file descriptor " << fd << " with event " << ev;
  throw std::runtime_error(oss.str());
}

void Client::free_tcp_socket(void)
{
  close(tcp_fd);
}

void Client::free_epoll(void)
{
  close(epoll_fd);
}