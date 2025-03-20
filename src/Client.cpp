/*================================================================================

File: Client.cpp                                                                
Creator: Claudio Raimondi                                                       
Email: claudio.raimondi@pm.me                                                   

created at: 2025-03-08 15:48:16                                                 
last edited: 2025-03-08 21:24:05                                                

================================================================================*/

#include <arpa/inet.h>
#include <netinet/tcp.h>
#include <unordered_map>
#include <byteswap.h>
#include <unistd.h>
#include <cstring>

#include "Client.hpp"
#include "Config.hpp"
#include "macros.hpp"

using namespace std::chrono_literals;

Client::Client(void) :
  config(),
  sanity_checker(),
  order_book(),
  glimpse_address(createAddress(config.glimpse_ip, config.glimpse_port)),
  multicast_address(createAddress(config.multicast_ip, config.multicast_port)),
  rewind_address(createAddress(config.rewind_ip, config.rewind_port)),
  tcp_sock_fd(createTcpSocket()),
  udp_sock_fd(createUdpSocket())
{
  bool error = false;

  const sockaddr_in bind_address_tcp = createAddress(config.bind_ip, config.bind_port_tcp);
  const sockaddr_in bind_address_udp = createAddress(config.bind_ip, config.bind_port_udp);

  error |= (bind(tcp_sock_fd, reinterpret_cast<const sockaddr *>(&bind_address_tcp), sizeof(bind_address_tcp) == -1));
  error |= (bind(udp_sock_fd, reinterpret_cast<const sockaddr *>(&bind_address_udp), sizeof(bind_address_udp) == -1));

  ip_mreq mreq{};
  mreq.imr_interface.s_addr = bind_address_udp.sin_addr.s_addr;
  mreq.imr_multiaddr.s_addr = multicast_address.sin_addr.s_addr;

  error |= (setsockopt(udp_sock_fd, IPPROTO_IP, IP_ADD_MEMBERSHIP, &mreq, sizeof(mreq)) == -1);
  error |= (connect(tcp_sock_fd, reinterpret_cast<const sockaddr *>(&glimpse_address), sizeof(glimpse_address)) == -1);

  if (error)
    SanityChecker::throwException("Error during socket setup");
}

COLD sockaddr_in Client::createAddress(const std::string_view ip_str, const std::string_view port_str) const noexcept
{
  const char *const ip = ip_str.data();
  const uint16_t port = std::stoi(port_str.data());

  sockaddr_in address{};

  address.sin_addr.s_addr = inet_addr(ip);
  address.sin_family = AF_INET;
  address.sin_port = bswap_16(port);

  return address;
}

COLD int Client::createTcpSocket(void) const noexcept
{
  bool error = false;

  const int sock_fd = socket(AF_INET, SOCK_STREAM, 0);
  error |= (sock_fd == -1);

  constexpr int enable = 1;
  error |= (setsockopt(sock_fd, IPPROTO_TCP, TCP_FASTOPEN, &enable, sizeof(enable) == -1));
  error |= (setsockopt(sock_fd, IPPROTO_TCP, TCP_NODELAY, &enable, sizeof(enable) == -1));
  error |= (setsockopt(sock_fd, SOL_SOCKET, SO_ZEROCOPY, &enable, sizeof(enable) == -1));

  if (error)
    SanityChecker::throwException("Error during socket setup");

  return sock_fd;
}

COLD int Client::createUdpSocket(void) const noexcept
{
  bool error = false;

  const int sock_fd = socket(AF_INET, SOCK_DGRAM, 0);
  error |= (sock_fd == -1);

  constexpr int enable = 1;
  constexpr int disable = 0;
  constexpr int priority = 255;

  error |= (setsockopt(sock_fd, SOL_SOCKET, SO_ZEROCOPY, &enable, sizeof(enable) == -1));
  error |= (setsockopt(sock_fd, SOL_SOCKET, SO_PRIORITY, &priority, sizeof(priority) == -1));
  error |= (setsockopt(sock_fd, IPPROTO_IP, IP_MULTICAST_LOOP, &disable, sizeof(disable) == -1));
  error |= (setsockopt(sock_fd, SOL_SOCKET, SO_BUSY_POLL, &enable, sizeof(enable) == -1));
  //TODO SO_RCVBUF (not < than MTU)
  //TODO SO_BINDTODEVICE
  //TODO IP_MULTICAST_IF
  //TODO SO_BUSY_POLL_BUDGET
  if (error)
    SanityChecker::throwException("Error during socket setup");

  return sock_fd;
}

Client::~Client(void) noexcept
{
  close(tcp_sock_fd);
  close(udp_sock_fd);
}

void Client::run(void)
{
  fetchOrderbook();
  updateOrderbook();
}

COLD void Client::fetchOrderbook(void)
{
  sendLogin();
  recvLogin();

  while (recvSnapshot() == false)
    continue;

  sendLogout();
}

HOT void Client::updateOrderbook(void)
{
  constexpr uint8_t MAX_MSGS = 32;
  constexpr uint16_t MTU = 1500;
  constexpr uint16_t MAX_MSG_SIZE = MTU - sizeof(MoldUDP64Header);

  mmsghdr msgs[MAX_MSGS];
  iovec iov[MAX_MSGS][2];
  MoldUDP64Header headers[MAX_MSGS];
  char payloads[MAX_MSGS][MAX_MSG_SIZE];

  //TODO constexpr
  for (uint8_t i = 0; i < MAX_MSGS; ++i)
  {
    iov[i][0] = { &headers[i], sizeof(headers[i]) };
    iov[i][1] = { payloads[i], sizeof(payloads[i]) };

    msgs[i].msg_hdr = {
      .msg_name = (void*)&multicast_address,
      .msg_namelen = sizeof(multicast_address),
      .msg_iov = iov[i],
      .msg_iovlen = 2,
      .msg_control = nullptr,
      .msg_controllen = 0
    };
  }

  while (true)
  {
    const uint8_t packets_count = recvmmsg(udp_sock_fd, msgs, MAX_MSGS, MSG_WAITFORONE, nullptr);
    sanity_checker.updateLastReceived();
    //TODO sanity checker checks flags (truncation, error)
    //TODO sanity checker adds msg_count to the sequence number
    //TODO process message blocks
  }
}

COLD bool Client::sendLogin(void)
{
  SoupBinTCPPacket packet{};

  packet.length = sizeof(packet.type) + sizeof(packet.login_request);
  packet.type = 'L';

  auto &login = packet.login_request;

  std::memcpy(login.username, config.username.data(), sizeof(login.username));
  std::memcpy(login.password, config.password.data(), sizeof(login.password));
  std::memset(login.requested_session, ' ', sizeof(login.requested_session));
  login.requested_sequence[0] = '1';

  constexpr uint16_t packet_size = sizeof(packet.length) + sizeof(packet.type) + sizeof(login);
  send(tcp_sock_fd, reinterpret_cast<const char *>(&packet), packet_size, 0);
  sanity_checker.updateLastSent();

  return true;
}

COLD bool Client::recvLogin(void)
{
  SoupBinTCPPacket packet;
  constexpr uint16_t header_size = sizeof(packet.length) + sizeof(packet.type);

  recv(tcp_sock_fd, reinterpret_cast<char *>(&packet), header_size, 0);
  sanity_checker.updateLastReceived();

  switch (packet.type)
  {
    case 'H':
      return recvLogin();
    case 'J':
      //TODO read the message body and get the reason
      SanityChecker::throwException("Login rejected");
      UNREACHABLE;
    case 'A':
    {
      const uint16_t payload_size = bswap_32(packet.length) - sizeof(packet.type);
      recv(tcp_sock_fd, reinterpret_cast<char *>(&packet.login_acceptance), payload_size, 0);
      sanity_checker.updateLastReceived();
      const uint64_t sequence_number = std::stoull(packet.login_acceptance.sequence);
      sanity_checker.setSequenceNumber(sequence_number);
      return true;
    }
    default:
      SanityChecker::throwException("Invalid packet type");
  }

  UNREACHABLE;
}

bool Client::recvSnapshot(void)
{
  SoupBinTCPPacket packet;
  constexpr uint16_t header_size = sizeof(packet.length) + sizeof(packet.type);

  recv(tcp_sock_fd, reinterpret_cast<char *>(&packet), header_size, 0);
  sanity_checker.updateLastReceived();

  switch (packet.type)
  {
    case 'H':
      return recvSnapshot();
    case 'S':
    {
      const uint16_t payload_size = bswap_32(packet.length) - sizeof(packet.type);
      std::vector<char> buffer(payload_size);
      recv(tcp_sock_fd, buffer.data(), payload_size, 0);
      sanity_checker.updateLastReceived();
      return processMessageBlocks(buffer);
    }
    default:
      SanityChecker::throwException("Invalid packet type");
  }

  UNREACHABLE;
}

COLD bool Client::sendLogout(void)
{
  constexpr SoupBinTCPPacket packet = {
    .length = 1,
    .type = 'Z',
    .logout_request{}
  };
  constexpr uint16_t packet_size = sizeof(packet.length) + sizeof(packet.type);

  send(tcp_sock_fd, reinterpret_cast<const char *>(&packet), packet_size, 0);
  sanity_checker.updateLastSent();

  return true;
}

bool Client::processMessageBlocks(const std::vector<char> &buffer)
{
  auto it = buffer.cbegin();
  const auto end = buffer.cend();

  while (it != end)
  {
    const MessageBlock &block = *reinterpret_cast<const MessageBlock *>(&*it);
    const uint16_t block_length = bswap_16(block.length);

    sanity_checker.validateMessageBlock(block);

    switch (block.type)
    {
      case 'A':
        handleNewOrder(block);
        break;
      case 'D':
        handleDeletedOrder(block);
        break;
      case 'G':
      {
        const uint64_t sequence_number = std::stoull(block.snapshot_completion.sequence);
        sanity_checker.checkSequenceNumber(sequence_number);
        return true;
      }
    }

    std::advance(it, block.length);
  }

  return false;
}

HOT void Client::handleNewOrder(const MessageBlock &block)
{
  const auto &new_order = block.new_order;
  const OrderBook::Side side = static_cast<OrderBook::Side>(new_order.side == 'S');
  const int32_t price = bswap_32(new_order.price);
  const uint64_t volume = bswap_32(new_order.quantity);

  if (price == INT32_MIN)
    order_book.executeOrder(side, volume);
  else
    order_book.addOrder(side, price, volume);
}

//TODO find a way to efficiently remove orders, and sanitize them before calling the orderbook
HOT void Client::handleDeletedOrder(const MessageBlock &block)
{
  (void)block;
  // const auto &deleted_order = block.deleted_order;
  // const OrderBook::Side side = static_cast<OrderBook::Side>(deleted_order.side == 'S');
  // const uint32_t price = bswap_32(deleted_order.price);
  // const uint64_t volume = bswap_32(deleted_order.quantity);

  // order_book.removeOrder(side, price, volume);
}