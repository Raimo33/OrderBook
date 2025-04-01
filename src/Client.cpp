/*================================================================================

File: Client.cpp                                                                
Creator: Claudio Raimondi                                                       
Email: claudio.raimondi@pm.me                                                   

created at: 2025-03-08 15:48:16                                                 
last edited: 2025-04-01 20:42:59                                                

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
  order_books(),
  username(username),
  password(password),
  glimpse_address(createAddress(GLIMPSE_IP, GLIMPSE_PORT)),
  multicast_address(createAddress(MULTICAST_IP, MULTICAST_PORT)),
  rewind_address(createAddress(REWIND_IP, REWIND_PORT)),
  bind_address_tcp(createAddress(BIND_IP, "0")),
  bind_address_udp(createAddress(BIND_IP, MULTICAST_PORT)),
  tcp_sock_fd(createTcpSocket()),
  udp_sock_fd(createUdpSocket()),
  sequence_number(0),
  status(CONNECTING)
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
  address.sin_port = htobe16(port);

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

  //TODO remove
  error |= setsockopt(sock_fd, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(enable)) == -1;
  error |= setsockopt(sock_fd, SOL_SOCKET, SO_REUSEPORT, &enable, sizeof(enable)) == -1;

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
  constexpr char ifname[] = IFNAME;
  constexpr int ifname_len = strlen(ifname);

  //TODO add
  // error |= setsockopt(sock_fd, SOL_SOCKET, SO_ZEROCOPY, &enable, sizeof(enable)) == -1;
  error |= setsockopt(sock_fd, SOL_SOCKET, SO_PRIORITY, &priority, sizeof(priority)) == -1;
  error |= setsockopt(sock_fd, IPPROTO_IP, IP_MULTICAST_LOOP, &disable, sizeof(disable)) == -1;
  error |= setsockopt(sock_fd, SOL_SOCKET, SO_BUSY_POLL, &enable, sizeof(enable)) == -1;
  error |= setsockopt(sock_fd, SOL_SOCKET, SO_RCVBUF, &recv_bufsize, sizeof(recv_bufsize)) == -1;
  error |= setsockopt(sock_fd, SOL_SOCKET, SO_BINDTODEVICE, ifname, ifname_len) == -1;
  error |= setsockopt(sock_fd, IPPROTO_IP, IP_MULTICAST_ALL, &disable, sizeof(disable)) == -1;

  //TODO remove
  error |= setsockopt(sock_fd, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(enable)) == -1;
  error |= setsockopt(sock_fd, SOL_SOCKET, SO_REUSEPORT, &enable, sizeof(enable)) == -1;

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
  fetchOrderbooks();

  printf("exiting for now\n");
  exit(EXIT_SUCCESS);

  updateOrderbooks();
}

COLD void Client::fetchOrderbooks(void)
{
  sendLogin();
  recvLogin();
  while (status == FETCHING)
    recvSnapshot();
  sendLogout();
}

HOT void Client::updateOrderbooks(void)
{
  constexpr uint16_t MAX_MSG_SIZE = MTU - sizeof(MoldUDP64Header);

  //+1 added for safe prefetching past the last packet 
  alignas(CACHELINE_SIZE) mmsghdr mmsgs[MAX_BURST_PACKETS+1]{};
  alignas(CACHELINE_SIZE) iovec iov[2][MAX_BURST_PACKETS+1]{};
  alignas(CACHELINE_SIZE) struct Packet {
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
      const uint64_t sequence_number = be64toh(packet->header.sequence_number);
      const uint16_t message_count = be16toh(packet->header.message_count);

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
  auto &body = packet.body;
  constexpr uint8_t body_length = sizeof(body.type) + sizeof(body.login_request);
  constexpr uint16_t packet_size = sizeof(packet.body_length) + body_length;

  packet.body_length = htobe16(body_length);

  body.type = 'L';
  std::memset(&body.login_request, ' ', sizeof(body.login_request));
  std::strncpy(body.login_request.username, username.data(), username.size());
  std::strncpy(body.login_request.password, password.data(), password.size());
  body.login_request.requested_sequence[0] = '1';

  error |= send(tcp_sock_fd, &packet, packet_size, MSG_WAITALL) == -1;
  CHECK_ERROR;
}

COLD void Client::recvLogin(void)
{
  SoupBinTCPPacket packet{};

  error |= recv(tcp_sock_fd, &packet, sizeof(packet.body_length), MSG_WAITALL) == -1;
  const uint16_t body_length = be16toh(packet.body_length);
  error |= recv(tcp_sock_fd, &packet.body, body_length, MSG_WAITALL) == -1;

  CHECK_ERROR;

  switch (packet.body.type)
  {
    case 'H':
      recvLogin();
      break;
    case 'A':
      sequence_number = std::stoull(packet.body.login_acceptance.sequence);
      status = FETCHING;
      break;
    default:
      panic();
  }
}

COLD void Client::recvSnapshot(void)
{
  thread_local static std::array<char, UINT16_MAX> buffer{};
  SoupBinTCPPacket &packet = *reinterpret_cast<SoupBinTCPPacket *>(buffer.data());

  error |= recv(tcp_sock_fd, &packet, sizeof(packet.body_length), MSG_WAITALL) == -1;
  const uint16_t body_length = be16toh(packet.body_length);
  error |= recv(tcp_sock_fd, &packet.body, body_length, MSG_WAITALL) == -1;

  CHECK_ERROR;

  switch (packet.body.type)
  {
    case 'H':
      recvSnapshot();
      break;
    case 'S':
    {
      const char *const payload = reinterpret_cast<const char *>(&packet.body.sequenced_data);
      processSnapshot(payload, body_length - sizeof(packet.body.type));
      break;
    }
    default:
      panic();
  }
}

COLD void Client::sendLogout(void) const
{
  const SoupBinTCPPacket packet = {
    .body_length = htobe16(1),
    .body = {
      .type = 'Z',
      .logout_request{}
    }
  };
  constexpr uint16_t packet_size = sizeof(packet.body_length) + sizeof(packet.body.type);

  error |= send(tcp_sock_fd, &packet, packet_size, MSG_WAITALL) == -1;
  CHECK_ERROR;
}

COLD void Client::processSnapshot(const char *restrict buffer, const uint16_t buffer_size)
{
  constexpr uint8_t size = 'Z' + 1;
  constexpr std::array<uint16_t, size> data_lengths = []()
  {
    std::array<uint16_t, size> data_lengths{};
    data_lengths['A'] = sizeof(MessageData::new_order);
    data_lengths['D'] = sizeof(MessageData::deleted_order);
    data_lengths['T'] = sizeof(MessageData::seconds);
    data_lengths['R'] = sizeof(MessageData::series_info_basic);
    data_lengths['M'] = sizeof(MessageData::series_info_basic_combination);
    data_lengths['L'] = sizeof(MessageData::tick_size_data);
    data_lengths['S'] = sizeof(MessageData::system_event);
    data_lengths['O'] = sizeof(MessageData::trading_status);
    data_lengths['E'] = sizeof(MessageData::execution_notice);
    data_lengths['C'] = sizeof(MessageData::execution_notice_with_trade_info);
    data_lengths['Z'] = sizeof(MessageData::ep);
    data_lengths['G'] = sizeof(MessageData::snapshot_completion);
    return data_lengths;
  }();

  const char *const end = buffer + buffer_size;

  while (buffer < end)
  {
    const MessageData &data = *reinterpret_cast<const MessageData *>(buffer);
    const uint16_t length = sizeof(data.type) + data_lengths[data.type];

    PREFETCH_R(buffer + length, 1);
    processMessageData(data);

    sequence_number++;
    buffer += length;
  }
}

HOT void Client::processMessageBlocks(const char *restrict buffer, uint16_t blocks_count)
{
  while (blocks_count--)
  {
    const MessageBlock &block = *reinterpret_cast<const MessageBlock *>(buffer);
    const uint16_t length = sizeof(block.length) + be16toh(block.length);

    PREFETCH_R(buffer + length, 1);
    processMessageData(block.data);

    buffer += length;
  }
}

HOT void Client::processMessageData(const MessageData &data)
{
  using MessageHandler = void (Client::*)(const MessageData &);

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
    handlers['G'] = &Client::handleSnapshotCompletion;
    return handlers;
  }();

  (this->*handlers[data.type])(data);
}

COLD void Client::handleSnapshotCompletion(const MessageData &data)
{
  const auto &snapshot_completion = data.snapshot_completion;
  sequence_number = std::stoull(snapshot_completion.sequence);
  status = UPDATING;
}

HOT void Client::handleNewOrder(const MessageData &data)
{
  const auto &new_order = data.new_order;
  const uint32_t book_id = be32toh(new_order.orderbook_id);
  const uint64_t order_id = be64toh(new_order.order_id);
  const OrderBook::Side side = static_cast<OrderBook::Side>(new_order.side);
  const int32_t price = be32toh(new_order.price);
  const uint64_t qty = be64toh(new_order.quantity);

  if (price == INT32_MIN)
    return;

  printf("adding order\n");

  order_books[book_id]->addOrder(order_id, side, price, qty);
}

HOT void Client::handleDeletedOrder(const MessageData &data)
{
  const auto &deleted_order = data.deleted_order;
  const uint32_t book_id = be32toh(deleted_order.orderbook_id);
  const uint64_t order_id = be64toh(deleted_order.order_id);
  const OrderBook::Side side = static_cast<OrderBook::Side>(deleted_order.side);

  printf("deleting order\n");

  order_books[book_id]->removeOrder(order_id, side);
}

HOT void Client::handleExecutionNotice(const MessageData &data)
{
  const auto &execution_notice = data.execution_notice;
  const uint32_t book_id = be32toh(execution_notice.orderbook_id);
  const uint64_t order_id = be64toh(execution_notice.order_id);
  const OrderBook::Side resting_side = static_cast<OrderBook::Side>(execution_notice.side);
  const uint64_t qty = be64toh(execution_notice.executed_quantity);

  printf("executing order\n");

  order_books[book_id]->executeOrder(order_id, resting_side, qty);
}

HOT void Client::handleExecutionNoticeWithTradeInfo(const MessageData &data)
{
  const auto &execution_notice = data.execution_notice_with_trade_info;
  const uint32_t book_id = be32toh(execution_notice.orderbook_id);
  const uint64_t order_id = be64toh(execution_notice.order_id);
  const OrderBook::Side resting_side = static_cast<OrderBook::Side>(execution_notice.side);
  const uint64_t qty = be64toh(execution_notice.executed_quantity);
  const int32_t price = be32toh(execution_notice.trade_price);

  printf("executing order with trade info\n");

  order_books[book_id]->removeOrder(order_id, resting_side, price, qty);
}

COLD void Client::handleEquilibriumPrice(const MessageData &data)
{
  const auto &equilibrium_price = data.ep;
  const uint32_t book_id = be32toh(equilibrium_price.orderbook_id);
  const int32_t price = be32toh(equilibrium_price.equilibrium_price);
  const uint64_t bid_qty = be64toh(equilibrium_price.available_bid_quantity);
  const uint64_t ask_qty = be64toh(equilibrium_price.available_ask_quantity);

  printf("setting equilibrium price\n");

  order_books[book_id]->setEquilibrium(price, bid_qty, ask_qty);
}

HOT void Client::handleSeconds(UNUSED const MessageData &data)
{
}

COLD void Client::handleSeriesInfoBasic(const MessageData &data)
{
  const uint32_t book_id = be32toh(data.series_info_basic.orderbook_id);
  const std::string_view symbol(data.series_info_basic.symbol, sizeof(data.series_info_basic.symbol));

  if (LIKELY(order_books.find(book_id) != order_books.end()))
    return;

  order_books.emplace(book_id, std::make_unique<OrderBook>(book_id, symbol));
}

COLD void Client::handleSeriesInfoBasicCombination(UNUSED const MessageData &data)
{
}

COLD void Client::handleTickSizeData(UNUSED const MessageData &data)
{
}

COLD void Client::handleSystemEvent(UNUSED const MessageData &data)
{
}

COLD void Client::handleTradingStatus(UNUSED const MessageData &data)
{
  //"M_ZARABA", "A_ZARABA_E", "A_ZARABA_E2", "N_ZARABA", "A_ZARABA"
  // if (status[2] == 'Z')
  //   resumeTrading();
}