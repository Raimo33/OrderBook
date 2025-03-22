/*================================================================================

File: Client.cpp                                                                
Creator: Claudio Raimondi                                                       
Email: claudio.raimondi@pm.me                                                   

created at: 2025-03-08 15:48:16                                                 
last edited: 2025-03-22 22:23:11                                                

================================================================================*/

#include <arpa/inet.h>
#include <netinet/tcp.h>
#include <array>
#include <byteswap.h>
#include <unistd.h>
#include <cstring>

#include "Client.hpp"
#include "Config.hpp"
#include "macros.hpp"
#include "error.hpp"

COLD Client::Client(void) :
  config(),
  order_book(),
  glimpse_address(createAddress(config.glimpse_ip, config.glimpse_port)),
  multicast_address(createAddress(config.multicast_ip, config.multicast_port)),
  rewind_address(createAddress(config.rewind_ip, config.rewind_port)),
  tcp_sock_fd(createTcpSocket()),
  udp_sock_fd(createUdpSocket()),
  sequence_number(0)
{
  const sockaddr_in bind_address_tcp = createAddress(config.bind_ip, config.bind_port_tcp);
  const sockaddr_in bind_address_udp = createAddress(config.bind_ip, config.bind_port_udp);

  error |= (bind(tcp_sock_fd, reinterpret_cast<const sockaddr *>(&bind_address_tcp), sizeof(bind_address_tcp)) == -1);
  error |= (bind(udp_sock_fd, reinterpret_cast<const sockaddr *>(&bind_address_udp), sizeof(bind_address_udp)) == -1);

  ip_mreq mreq{};
  mreq.imr_interface.s_addr = bind_address_udp.sin_addr.s_addr;
  mreq.imr_multiaddr.s_addr = multicast_address.sin_addr.s_addr;

  error |= (setsockopt(udp_sock_fd, IPPROTO_IP, IP_ADD_MEMBERSHIP, &mreq, sizeof(mreq)) == -1);
  error |= (connect(tcp_sock_fd, reinterpret_cast<const sockaddr *>(&glimpse_address), sizeof(glimpse_address)) == -1);

  CHECK_ERROR;
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
  const int sock_fd = socket(AF_INET, SOCK_STREAM, 0);
  error |= (sock_fd == -1);

  constexpr int enable = 1;

  error |= (setsockopt(sock_fd, IPPROTO_TCP, TCP_FASTOPEN, &enable, sizeof(enable)) == -1);
  error |= (setsockopt(sock_fd, IPPROTO_TCP, TCP_NODELAY, &enable, sizeof(enable)) == -1);
  error |= (setsockopt(sock_fd, SOL_SOCKET, SO_ZEROCOPY, &enable, sizeof(enable)) == -1);

  CHECK_ERROR;

  return sock_fd;
}

COLD int Client::createUdpSocket(void) const noexcept
{
  const int sock_fd = socket(AF_INET, SOCK_DGRAM, 0);
  error |= (sock_fd == -1);

  constexpr int enable = 1;
  constexpr int disable = 0;
  constexpr int priority = 255;

  error |= (setsockopt(sock_fd, SOL_SOCKET, SO_ZEROCOPY, &enable, sizeof(enable)) == -1);
  error |= (setsockopt(sock_fd, SOL_SOCKET, SO_PRIORITY, &priority, sizeof(priority)) == -1);
  error |= (setsockopt(sock_fd, IPPROTO_IP, IP_MULTICAST_LOOP, &disable, sizeof(disable)) == -1);
  error |= (setsockopt(sock_fd, SOL_SOCKET, SO_BUSY_POLL, &enable, sizeof(enable)) == -1);
  //TODO SO_RCVBUF (not < than MTU)
  //TODO SO_BINDTODEVICE
  //TODO IP_MULTICAST_IF
  //TODO SO_BUSY_POLL_BUDGET

  CHECK_ERROR;

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
  constexpr uint8_t MAX_PACKETS = 64;
  constexpr uint16_t MTU = 1500;
  constexpr uint16_t MAX_MSG_SIZE = MTU - sizeof(MoldUDP64Header);

  //+1 added for safe prefetching past the last packet 
  alignas(64) mmsghdr packets[MAX_PACKETS+1];
  alignas(64) iovec iov[MAX_PACKETS+1][2];
  alignas(64) MoldUDP64Header headers[MAX_PACKETS+1];
  alignas(64) char payloads[MAX_PACKETS+1][MAX_MSG_SIZE];

  for (uint8_t i = 0; i < MAX_PACKETS; ++i)
  {
    iov[i][0] = { &headers[i], sizeof(headers[i]) };
    iov[i][1] = { payloads[i], sizeof(payloads[i]) };

    msghdr &packet = packets[i].msg_hdr;
    packet.msg_name = (void*)&multicast_address;
    packet.msg_namelen = sizeof(multicast_address);
    packet.msg_iov = iov[i];
    packet.msg_iovlen = 2;
  }

  while (true)
  {
    int8_t packets_count = recvmmsg(udp_sock_fd, packets, MAX_PACKETS, MSG_WAITFORONE, nullptr);
    error |= (packets_count == -1);

    const MoldUDP64Header *header_ptr = headers;
    const char *payload_ptr = reinterpret_cast<char *>(payloads);

    while (packets_count--)
    {
      PREFETCH_R(header_ptr + 1, 2);
      PREFETCH_R(payload_ptr + MAX_MSG_SIZE, 2);

      const uint64_t sequence_number = bswap_64(header_ptr->sequence_number);
      const uint16_t message_count = bswap_16(header_ptr->message_count);

      error |= (sequence_number != this->sequence_number);
      CHECK_ERROR;

      processMessageBlocks(payload_ptr, message_count);

      header_ptr++;
      payload_ptr += MAX_MSG_SIZE;
    }
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
  error |= (send(tcp_sock_fd, reinterpret_cast<const char *>(&packet), packet_size, 0) == -1);
  CHECK_ERROR;

  return true;
}

COLD bool Client::recvLogin(void)
{
  SoupBinTCPPacket packet;
  constexpr uint16_t header_size = sizeof(packet.length) + sizeof(packet.type);

  error |= (recv(tcp_sock_fd, reinterpret_cast<char *>(&packet), header_size, 0) == -1);
  CHECK_ERROR;

  switch (packet.type)
  {
    case 'H':
      return recvLogin();
    case 'A':
    {
      const uint16_t payload_size = bswap_32(packet.length) - sizeof(packet.type);
      error |= (recv(tcp_sock_fd, reinterpret_cast<char *>(&packet.login_acceptance), payload_size, 0) == -1);
      sequence_number = std::stoull(packet.login_acceptance.sequence);
      return true;
    }
    default:
      error = true;
  }
}

bool Client::recvSnapshot(void)
{
  SoupBinTCPPacket packet;
  constexpr uint16_t header_size = sizeof(packet.length) + sizeof(packet.type);

  error |= (recv(tcp_sock_fd, reinterpret_cast<char *>(&packet), header_size, 0) == -1);
  CHECK_ERROR;

  switch (packet.type)
  {
    case 'H':
      return recvSnapshot();
    case 'S':
    {
      const uint16_t payload_size = bswap_32(packet.length) - sizeof(packet.type);
      std::vector<char> buffer(payload_size);
      error |= (recv(tcp_sock_fd, buffer.data(), payload_size, 0) == -1);
      return processMessageBlocks(buffer);
    }
    default:
      error = true;
  }
}

COLD bool Client::sendLogout(void)
{
  constexpr SoupBinTCPPacket packet = {
    .length = 1,
    .type = 'Z',
    .logout_request{}
  };
  constexpr uint16_t packet_size = sizeof(packet.length) + sizeof(packet.type);

  error |= (send(tcp_sock_fd, reinterpret_cast<const char *>(&packet), packet_size, 0) == -1);
  CHECK_ERROR;

  return true;
}

bool Client::processMessageBlocks(const std::vector<char> &buffer)
{
  auto it = buffer.cbegin();
  const auto end = buffer.cend();

  while (it != end)
  {
    const MessageBlock *block = reinterpret_cast<const MessageBlock *>(&*it);
    const uint16_t block_length = bswap_16(block->length);

    switch (block->type)
    {
      case 'A':
        handleNewOrder(*block);
        break;
      case 'D':
        handleDeletedOrder(*block);
        break;
      case 'G':
      {
        const uint64_t sequence_number = std::stoull(block->snapshot_completion.sequence);
        error |= (sequence_number != ++this->sequence_number);
        CHECK_ERROR;
        return true;
      }
    }

    sequence_number++;
    std::advance(it, block_length);
  }

  return false;
}

HOT void Client::processMessageBlocks(const char *buffer, uint16_t blocks_count)
{
  using MessageHandler = void (Client::*)(const MessageBlock &);
  
    constexpr std::array<MessageHandler, 256> handlers = []()
  {
    std::array<MessageHandler, 256> handlers{};
    handlers['A'] = &Client::handleNewOrder;
    handlers['D'] = &Client::handleDeletedOrder;
    handlers['T'] = &Client::handleSeconds;
    handlers['R'] = &Client::handleSeriesInfoBasic;
    handlers['M'] = &Client::handleSeriesInfoBasicCombination;
    handlers['L'] = &Client::handleTickSizeData;
    handlers['S'] = &Client::handleSystemEvent;
    handlers['O'] = &Client::handleTradingStatus;
    handlers['E'] = &Client::handleExecutionNotice;
    handlers['C'] = &Client::handleExecutionNoticeWithTradeInfo;
    handlers['Z'] = &Client::handleEquilibriumPrice;
    return handlers;
  }();

  while (blocks_count--)
  {
    const MessageBlock *block = reinterpret_cast<const MessageBlock *>(buffer);
    const uint16_t block_length = bswap_16(block->length);

    PREFETCH_R(buffer + block_length, 3);

    (this->*handlers[block->type])(*block);

    sequence_number++;
    buffer += block_length;
  }
}

HOT void Client::handleNewOrder(const MessageBlock &block)
{
  const auto &new_order = block.new_order;
  const OrderBook::Side side = static_cast<OrderBook::Side>(new_order.side == 'S');
  const int32_t price = bswap_32(new_order.price);
  const uint64_t volume = bswap_32(new_order.quantity);

  using OrderHandler = void (OrderBook::*)(const OrderBook::Side, const uint32_t, const uint64_t);
  alignas(64) constexpr std::array<OrderHandler, 2> handlers = {
    &OrderBook::addOrder,
    &OrderBook::removeOrder
  }; 

  const uint8_t idx = (price == INT32_MIN);
  (order_book.*handlers[idx])(side, price, volume);
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

HOT void Client::handleSeconds(const MessageBlock &block)
{
  (void)block;
}

COLD void Client::handleSeriesInfoBasic(const MessageBlock &block)
{
  (void)block;
}

COLD void Client::handleSeriesInfoBasicCombination(const MessageBlock &block)
{
  (void)block;
}

COLD void Client::handleTickSizeData(const MessageBlock &block)
{
  (void)block;
}

COLD void Client::handleSystemEvent(const MessageBlock &block)
{
  (void)block;
}

COLD void Client::handleTradingStatus(const MessageBlock &block)
{
  (void)block;
}

HOT void Client::handleExecutionNotice(const MessageBlock &block)
{
  (void)block;
}

HOT void Client::handleExecutionNoticeWithTradeInfo(const MessageBlock &block)
{
  (void)block;
}

COLD void Client::handleEquilibriumPrice(const MessageBlock &block)
{
  (void)block;
}