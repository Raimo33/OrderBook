/*================================================================================

File: Client.cpp                                                                
Creator: Claudio Raimondi                                                       
Email: claudio.raimondi@pm.me                                                   

created at: 2025-03-08 15:48:16                                                 
last edited: 2025-03-30 20:08:25                                                

================================================================================*/

#include <arpa/inet.h>
#include <netinet/tcp.h>
#include <array>
#include <endian.h>
#include <unistd.h>
#include <cstring>

#include "Client.hpp"
#include "config.hpp"
#include "macros.hpp"
#include "error.hpp"

COLD Client::Client(const std::string_view username, const std::string_view password) noexcept :
  username(username),
  password(password),
  order_book(),
  glimpse_address(createAddress(GLIMPSE_IP, GLIMPSE_PORT)),
  multicast_address(createAddress(MULTICAST_IP, MULTICAST_PORT)),
  rewind_address(createAddress(REWIND_IP, REWIND_PORT)),
  bind_address_tcp(createAddress(BIND_IP, "0")),
  bind_address_udp(createAddress(BIND_IP, MULTICAST_PORT)),
  tcp_sock_fd(createTcpSocket()),
  udp_sock_fd(createUdpSocket()),
  sequence_number(0)
{
  error |= bind(tcp_sock_fd, reinterpret_cast<const sockaddr *>(&bind_address_tcp), sizeof(bind_address_tcp)) == -1;
  error |= bind(udp_sock_fd, reinterpret_cast<const sockaddr *>(&bind_address_udp), sizeof(bind_address_udp)) == -1;

  ip_mreq mreq{};
  mreq.imr_interface.s_addr = bind_address_udp.sin_addr.s_addr;
  mreq.imr_multiaddr.s_addr = multicast_address.sin_addr.s_addr;

  error |= setsockopt(udp_sock_fd, IPPROTO_IP, IP_ADD_MEMBERSHIP, &mreq, sizeof(mreq)) == -1;
  error |= connect(tcp_sock_fd, reinterpret_cast<const sockaddr *>(&glimpse_address), sizeof(glimpse_address)) == -1;

  CHECK_ERROR;
}

COLD sockaddr_in Client::createAddress(const std::string_view ip_str, const std::string_view port_str) const noexcept
{
  const char *const ip = ip_str.data();
  const uint16_t port = std::stoi(port_str.data());

  sockaddr_in address{};

  address.sin_addr.s_addr = inet_addr(ip);
  address.sin_family = AF_INET;
  address.sin_port = be16toh(port);

  return address;
}

COLD int Client::createTcpSocket(void) const noexcept
{
  const int sock_fd = socket(AF_INET, SOCK_STREAM, 0);
  error |= sock_fd == -1;

  constexpr int enable = 1;

  error |= setsockopt(sock_fd, IPPROTO_TCP, TCP_FASTOPEN, &enable, sizeof(enable)) == -1;
  error |= setsockopt(sock_fd, IPPROTO_TCP, TCP_NODELAY, &enable, sizeof(enable)) == -1;
  error |= setsockopt(sock_fd, SOL_SOCKET, SO_ZEROCOPY, &enable, sizeof(enable)) == -1;

  CHECK_ERROR;

  return sock_fd;
}

COLD int Client::createUdpSocket(void) const noexcept
{
  const int sock_fd = socket(AF_INET, SOCK_DGRAM, 0);
  error |= sock_fd == -1;

  constexpr int enable = 1;
  constexpr int disable = 0;
  constexpr int priority = 255;
  constexpr int recv_bufsize = SOCK_BUFSIZE;
  constexpr char *ifname = IFNAME;
  constexpr int ifname_len = strlen(ifname);

  error |= setsockopt(sock_fd, SOL_SOCKET, SO_ZEROCOPY, &enable, sizeof(enable)) == -1;
  error |= setsockopt(sock_fd, SOL_SOCKET, SO_PRIORITY, &priority, sizeof(priority)) == -1;
  error |= setsockopt(sock_fd, IPPROTO_IP, IP_MULTICAST_LOOP, &disable, sizeof(disable)) == -1;
  error |= setsockopt(sock_fd, SOL_SOCKET, SO_BUSY_POLL, &enable, sizeof(enable)) == -1;
  error |= setsockopt(sock_fd, SOL_SOCKET, SO_RCVBUF, &recv_bufsize, sizeof(recv_bufsize)) == -1;
  error |= setsockopt(sock_fd, SOL_SOCKET, SO_BINDTODEVICE, ifname, ifname_len) == -1;
  error |= setsockopt(sock_fd, IPPROTO_IP, IP_MULTICAST_ALL, &disable, sizeof(disable)) == -1;

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
  recvSnapshot();
  sendLogout();
}

HOT void Client::updateOrderbook(void)
{
  constexpr uint16_t MAX_MSG_SIZE = MTU - sizeof(MoldUDP64Header);

  //+1 added for safe prefetching past the last packet 
  alignas(64) mmsghdr mmsgs[MAX_BURST_PACKETS+1]{};
  alignas(64) iovec iov[2][MAX_BURST_PACKETS+1]{};
  alignas(64) struct Packet {
    MoldUDP64Header header;
    char payload[MAX_MSG_SIZE];
  } packets[MAX_BURST_PACKETS+1]{};

  for (int i = 0; i < MAX_BURST_PACKETS; ++i)
  {
    iov[0][i] = { &packets[i].header, sizeof(MoldUDP64Header) };
    iov[1][i] = { packets[i].payload, MAX_MSG_SIZE };

    mmsgs[i].msg_hdr.msg_iov = &iov[0][i];
    mmsgs[i].msg_hdr.msg_iovlen = 2;
  }

  while (true)
  {
    int8_t packets_count = recvmmsg(udp_sock_fd, mmsgs, MAX_BURST_PACKETS, MSG_WAITFORONE, nullptr);
    error |= packets_count == -1;
    const Packet *packet = packets;

    while(packets_count--)
    {
      PREFETCH_R(packet + 1, 1);
      const MoldUDP64Header &header = packet->header;
      const uint64_t sequence_number = be64toh(header.sequence_number);
      const uint16_t message_count = be16toh(header.message_count);

      error |= (sequence_number != this->sequence_number);
      CHECK_ERROR;

      processMessageBlocks(packet->payload, message_count);

      this->sequence_number += message_count;
      packet++;
    }
  }
  UNREACHABLE;
}

COLD void Client::sendLogin(void) const
{
  SoupBinTCPPacket packet{};

  packet.length = sizeof(packet.type) + sizeof(packet.login_request);
  packet.type = 'L';

  auto &login = packet.login_request;

  std::strcpy(login.username, username.data());
  std::strcpy(login.password, password.data());
  std::memset(login.requested_session, ' ', sizeof(login.requested_session));
  login.requested_sequence[0] = '1';

  constexpr uint16_t packet_size = sizeof(packet.length) + sizeof(packet.type) + sizeof(login);
  error |= send(tcp_sock_fd, &packet, packet_size, 0) == -1;
  CHECK_ERROR;
}

COLD void Client::recvLogin(void)
{
  SoupBinTCPPacket packet{};
  constexpr uint16_t header_size = sizeof(packet.length) + sizeof(packet.type);

  error |= recv(tcp_sock_fd, &packet, header_size, 0) == -1;
  CHECK_ERROR;

  switch (packet.type)
  {
    case 'H':
      recvLogin();
      break;
    case 'A':
    {
      error |= recv(tcp_sock_fd, &packet.login_acceptance, sizeof(packet.login_acceptance), 0) == -1;
      CHECK_ERROR;
      sequence_number = std::stoull(packet.login_acceptance.sequence);
      break;
    }
    default:
      panic();
  }
}

void Client::recvSnapshot(void)
{
  SoupBinTCPPacket packet{};
  constexpr uint16_t header_size = sizeof(packet.length) + sizeof(packet.type);

  error |= recv(tcp_sock_fd, reinterpret_cast<char *>(&packet), header_size, 0) == -1;
  CHECK_ERROR;

  switch (packet.type)
  {
    case 'H':
      break;
    case 'S':
    {
      const uint16_t payload_size = be32toh(packet.length) - sizeof(packet.type);
      std::vector<char> buffer(payload_size);
      error |= recv(tcp_sock_fd, buffer.data(), payload_size, 0) == -1;
      CHECK_ERROR;

      if (UNLIKELY(processMessageBlocks(buffer)))
        return;
      
      break;
    }
    default:
      panic();
  }

  recvSnapshot();
}

COLD void Client::sendLogout(void) const
{
  constexpr SoupBinTCPPacket packet = {
    .length = 1,
    .type = 'Z',
    .logout_request{}
  };
  constexpr uint16_t packet_size = sizeof(packet.length) + sizeof(packet.type);

  error |= send(tcp_sock_fd, reinterpret_cast<const char *>(&packet), packet_size, 0) == -1;
  CHECK_ERROR;
}

bool Client::processMessageBlocks(const std::vector<char> &buffer)
{
  auto it = buffer.cbegin();
  const auto end = buffer.cend();

  while (it != end)
  {
    const MessageBlock &block = *reinterpret_cast<const MessageBlock *>(&*it);
    const uint16_t block_length = be16toh(block.length);

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

HOT void Client::processMessageBlocks(const char *restrict buffer, uint16_t blocks_count)
{
  using MessageHandler = void (Client::*)(const MessageBlock &);

  constexpr uint8_t size = 'Z' + 1;
  constexpr std::array<MessageHandler, size> handlers = []()
  {
    std::array<MessageHandler, size> handlers{};
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
    const MessageBlock &block = *reinterpret_cast<const MessageBlock *>(buffer);
    const uint16_t block_length = be16toh(block.length);

    PREFETCH_R(buffer + block_length + sizeof(block.length), 3);

    (this->*handlers[block.type])(block);

    buffer += block_length + sizeof(block.length);
  }
}

HOT void Client::handleNewOrder(const MessageBlock &block)
{
  const auto &new_order = block.new_order;
  const uint64_t order_id = be64toh(new_order.order_id);
  const OrderBook::Side side = static_cast<OrderBook::Side>(new_order.side == 'S');
  const int32_t price = be32toh(new_order.price);
  const uint64_t qty = be64toh(new_order.quantity);

  if (price == INT32_MIN)
    return;

  order_book.addOrder(order_id, side, price, qty);
  //TODO push to the strategy executor.
}

HOT void Client::handleDeletedOrder(const MessageBlock &block)
{
  const auto &deleted_order = block.deleted_order;
  const uint64_t order_id = be64toh(deleted_order.order_id);
  const OrderBook::Side side = static_cast<OrderBook::Side>(deleted_order.side == 'S');

  order_book.removeOrder(order_id, side);
  //TODO push to the strategy executor.
}

HOT void Client::handleExecutionNotice(const MessageBlock &block)
{
  const auto &execution_notice = block.execution_notice;
  const uint64_t order_id = be64toh(execution_notice.order_id);
  const OrderBook::Side resting_side = static_cast<OrderBook::Side>(execution_notice.side == 'S');
  const uint64_t qty = be64toh(execution_notice.executed_quantity);

  order_book.executeOrder(order_id, resting_side, qty);
  //TODO push to the strategy executor.
}

HOT void Client::handleExecutionNoticeWithTradeInfo(const MessageBlock &block)
{
  const auto &execution_notice = block.execution_notice_with_trade_info;
  const uint64_t order_id = be64toh(execution_notice.order_id);
  const OrderBook::Side resting_side = static_cast<OrderBook::Side>(execution_notice.side == 'S');
  const uint64_t qty = be64toh(execution_notice.executed_quantity);
  const int32_t price = be32toh(execution_notice.trade_price);

  order_book.removeOrder(order_id, resting_side, price, qty);
  //TODO push to the strategy executor.
}

COLD void Client::handleEquilibriumPrice(const MessageBlock &block)
{
  const auto &equilibrium_price = block.ep;
  const int32_t price = be32toh(equilibrium_price.equilibrium_price);
  const uint64_t bid_qty = be64toh(equilibrium_price.available_bid_quantity);
  const uint64_t ask_qty = be64toh(equilibrium_price.available_ask_quantity);

  order_book.setEquilibrium(price, bid_qty, ask_qty);
  //TODO push to the strategy executor.
}

HOT void Client::handleSeconds(UNUSED const MessageBlock &block)
{
}

COLD void Client::handleSeriesInfoBasic(UNUSED const MessageBlock &block)
{
}

COLD void Client::handleSeriesInfoBasicCombination(UNUSED const MessageBlock &block)
{
}

COLD void Client::handleTickSizeData(UNUSED const MessageBlock &block)
{
}

COLD void Client::handleSystemEvent(UNUSED const MessageBlock &block)
{
}

COLD void Client::handleTradingStatus(UNUSED const MessageBlock &block)
{
  //"M_ZARABA", "A_ZARABA_E", "A_ZARABA_E2", "N_ZARABA", "A_ZARABA"
  // if (status[2] == 'Z')
  //   resumeTrading();
}