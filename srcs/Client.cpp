/*================================================================================

File: Client.cpp                                                                
Creator: Claudio Raimondi                                                       
Email: claudio.raimondi@pm.me                                                   

created at: 2025-03-08 15:48:16                                                 
last edited: 2025-05-03 19:03:11                                                

================================================================================*/

#include <arpa/inet.h>
#include <netinet/tcp.h>
#include <array>
#include <endian.h>
#include <unistd.h>
#include <cstring>
#include <memory>
#include <utility>

#include "Client.hpp"
#include "Config.hpp"
#include "utils/utils.hpp"
#include "macros.hpp"
#include "error.hpp"

COLD Client::Client(const Config &config) noexcept :
  username(config.username),
  password(config.password),
  glimpse_address(createAddress(config.glimpse_ip, config.glimpse_port)),
  multicast_address(createAddress(config.multicast_ip, config.multicast_port)),
  rewind_address(createAddress(config.rewind_ip, config.rewind_port)),
  bind_address_tcp(createAddress(config.bind_ip, "0")),
  bind_address_udp(createAddress(config.bind_ip, config.multicast_port)),
  tcp_sock_fd(createTcpSocket()),
  udp_sock_fd(createUdpSocket()),
  sequence_number(0),
  status(CONNECTING)
{
  for (const auto &id : config.book_ids)
    message_handler.addBookId(id);

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

  inet_pton(AF_INET, ip, &address.sin_addr);
  address.sin_family = AF_INET;
  address.sin_port = htons(port);

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
  constexpr int priority = 6;
  constexpr int recv_bufsize = SOCK_BUFSIZE;

  //TODO add
  // error |= setsockopt(sock_fd, SOL_SOCKET, SO_ZEROCOPY, &enable, sizeof(enable)) == -1;
  error |= setsockopt(sock_fd, SOL_SOCKET, SO_PRIORITY, &priority, sizeof(priority)) == -1;
  error |= setsockopt(sock_fd, IPPROTO_IP, IP_MULTICAST_LOOP, &disable, sizeof(disable)) == -1;
  error |= setsockopt(sock_fd, SOL_SOCKET, SO_BUSY_POLL, &enable, sizeof(enable)) == -1;
  error |= setsockopt(sock_fd, SOL_SOCKET, SO_RCVBUF, &recv_bufsize, sizeof(recv_bufsize)) == -1;
  error |= setsockopt(sock_fd, IPPROTO_IP, IP_MULTICAST_ALL, &disable, sizeof(disable)) == -1;

  //TODO remove
  error |= setsockopt(sock_fd, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(enable)) == -1;
  error |= setsockopt(sock_fd, SOL_SOCKET, SO_REUSEPORT, &enable, sizeof(enable)) == -1;

  CHECK_ERROR;

  return sock_fd;
}

COLD Client::~Client() noexcept
{
  close(tcp_sock_fd);
  close(udp_sock_fd);
}

COLD void Client::run(void)
{
  fetchOrderbooks();
  syncSequences();
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

COLD void Client::syncSequences(void)
{
  static constexpr uint16_t MAX_MSG_SIZE = MTU - sizeof(MoldUDP64Header);

  msghdr msg{};
  iovec iov[2];
  struct Packet {
    MoldUDP64Header header;
    char payload[MAX_MSG_SIZE];
  } packet{};

  iov[0] = { &packet.header, sizeof(MoldUDP64Header) };
  iov[1] = { packet.payload, MAX_MSG_SIZE };
  msg.msg_iov = iov;
  msg.msg_iovlen = 2;

  uint64_t sequence_number = 0;
  while (sequence_number < this->sequence_number - 1)
  {
    recvmsg(udp_sock_fd, &msg, MSG_WAITFORONE);
    sequence_number = packet.header.sequence_number;
  }
}

HOT void Client::updateOrderbooks(void)
{
  static constexpr uint16_t MAX_MSG_SIZE = MTU - sizeof(MoldUDP64Header);

  //+1 added for safe prefetching past the last packet 
  alignas(CACHELINE_SIZE) mmsghdr mmsgs[MAX_BURST_PACKETS+1];
  alignas(CACHELINE_SIZE) iovec iov[2][MAX_BURST_PACKETS+1];
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
    const Packet *packet = std::assume_aligned<CACHELINE_SIZE>(packets);

    while(packets_count--)
    {
      PREFETCH_R(packet + 1, 1);
      const uint16_t message_count = packet->header.message_count;

      error |= (packet->header.sequence_number != sequence_number);
      CHECK_ERROR;

      processMessageBlocks(packet->payload, message_count);

      sequence_number += message_count;
      packet++;
    }
  }

  std::unreachable();
}

COLD void Client::sendLogin(void) const
{
  SoupBinTCPPacket packet;
  auto &body = packet.body;
  static constexpr uint16_t body_length = sizeof(body.type) + sizeof(body.login_request);
  static constexpr uint16_t packet_size = sizeof(packet.body_length) + body_length;

  packet.body_length = body_length;

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
  SoupBinTCPPacket packet;

  error |= recv(tcp_sock_fd, &packet, sizeof(packet.body_length), MSG_WAITALL) == -1;
  error |= recv(tcp_sock_fd, &packet.body, packet.body_length, MSG_WAITALL) == -1;

  CHECK_ERROR;

  switch (packet.body.type)
  {
    case 'H':
      recvLogin();
      break;
    case 'A':
    {
      const auto &response = packet.body.login_acceptance;
      std::string sequence_str = std::string(response.sequence, sizeof(response.sequence));
      sequence_number = std::stoull(sequence_str);
      status = FETCHING;
      break;
    }
    default:
      panic();
  }
}

COLD void Client::recvSnapshot(void)
{
  thread_local static std::array<char, UINT16_MAX> buffer{};
  SoupBinTCPPacket &packet = *reinterpret_cast<SoupBinTCPPacket *>(buffer.data());

  error |= recv(tcp_sock_fd, &packet, sizeof(packet.body_length), MSG_WAITALL) == -1;
  error |= recv(tcp_sock_fd, &packet.body, packet.body_length, MSG_WAITALL) == -1;

  CHECK_ERROR;

  switch (packet.body.type)
  {
    case 'H':
      recvSnapshot();
      break;
    case 'S':
    {
      const char *const payload = reinterpret_cast<const char *>(&packet.body.sequenced_data);
      processSnapshots(payload, packet.body_length - sizeof(packet.body.type));
      break;
    }
    default:
      panic();
  }
}

COLD void Client::sendLogout(void) const
{
  SoupBinTCPPacket packet;
  packet.body_length = sizeof(packet.body.type);
  packet.body.type = 'Z';

  static constexpr uint16_t packet_size = sizeof(packet.body_length) + sizeof(packet.body.type);
  error |= send(tcp_sock_fd, &packet, packet_size, MSG_WAITALL) == -1;
  CHECK_ERROR;
}

COLD void Client::processSnapshots(const char *restrict buffer, const uint16_t buffer_size)
{
  static constexpr uint8_t size = 'Z' + 1;
  static constexpr std::array<uint16_t, size> data_lengths = []()
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

    if (data.type == 'G') [[unlikely]]
      handleSnapshotCompletion(data);
    else
      message_handler.handleMessage(data);

    sequence_number++;
    buffer += length;
  }
}

HOT void Client::processMessageBlocks(const char *restrict buffer, uint16_t blocks_count)
{
  while (blocks_count--)
  {
    const MessageBlock &block = *reinterpret_cast<const MessageBlock *>(buffer);
    const uint16_t length = sizeof(block.length) + block.length;

    PREFETCH_R(buffer + length, 1);
    message_handler.handleMessage(block.data);

    buffer += length;
  }
}

COLD void Client::handleSnapshotCompletion(const MessageData &data)
{
  const auto &snapshot_completion = data.snapshot_completion;
  sequence_number = std::stoull(snapshot_completion.sequence);
  status = UPDATING;
}

MessageHandler Client::message_handler{};
