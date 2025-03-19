/*================================================================================

File: Client.cpp                                                                
Creator: Claudio Raimondi                                                       
Email: claudio.raimondi@pm.me                                                   

created at: 2025-03-08 15:48:16                                                 
last edited: 2025-03-08 21:24:05                                                

================================================================================*/

#include <arpa/inet.h>
#include <netinet/tcp.h>
#include <sys/poll.h>
#include <sys/timerfd.h>
#include <cstring>
#include <array>
#include <unordered_map>

#include "Client.hpp"
#include "Config.hpp"
#include "utils.hpp"
#include "macros.hpp"

using namespace std::chrono_literals;

Client::Client(void) :
  order_book(),
  glimpse_address(createAddress(config.glimpse_ip, config.glimpse_port)),
  multicast_address(createAddress(config.multicast_ip, config.multicast_port)),
  rewind_address(createAddress(config.rewind_ip, config.rewind_port)),
  tcp_sock_fd(createTcpSocket()),
  udp_sock_fd(createUdpSocket())
{
  const sockaddr_in bind_address_tcp = createAddress(config.bind_ip, config.bind_port_tcp);
  const sockaddr_in bind_address_udp = createAddress(config.bind_ip, config.bind_port_udp);

  bind(tcp_sock_fd, reinterpret_cast<const sockaddr *>(&bind_address_tcp), sizeof(bind_address_tcp));
  bind(udp_sock_fd, reinterpret_cast<const sockaddr *>(&bind_address_udp), sizeof(bind_address_udp));

  ip_mreq mreq{};
  mreq.imr_interface.s_addr = bind_address_udp.sin_addr.s_addr;
  mreq.imr_multiaddr.s_addr = multicast_address.sin_addr.s_addr;

  setsockopt(udp_sock_fd, IPPROTO_IP, IP_ADD_MEMBERSHIP, &mreq, sizeof(mreq));
  connect(tcp_sock_fd, reinterpret_cast<const sockaddr *>(&glimpse_address), sizeof(glimpse_address));
}

COLD sockaddr_in Client::createAddress(const std::string_view ip_str, const std::string_view port_str) const noexcept
{
  const char *const ip = ip_str.data();
  const uint16_t port = std::stoi(port_str.data());

  sockaddr_in address{};

  address.sin_addr.s_addr = inet_addr(ip);
  address.sin_family = AF_INET;
  address.sin_port = utils::bswap16(port);

  return address;
}

COLD int Client::createTcpSocket(void) const noexcept
{
  const int sock_fd = socket(AF_INET, SOCK_STREAM, 0);

  constexpr int enable = 1;
  setsockopt(sock_fd, IPPROTO_TCP, TCP_FASTOPEN, &enable, sizeof(enable));
  setsockopt(sock_fd, IPPROTO_TCP, TCP_NODELAY, &enable, sizeof(enable));
  setsockopt(sock_fd, SOL_SOCKET, SO_ZEROCOPY, &enable, sizeof(enable));

  return sock_fd;
}

COLD int Client::createUdpSocket(void) const noexcept
{
  const int sock_fd = socket(AF_INET, SOCK_DGRAM, 0);

  constexpr int enable = 1;
  constexpr int priority = 255;
  setsockopt(sock_fd, SOL_SOCKET, SO_ZEROCOPY, &enable, sizeof(enable));
  setsockopt(sock_fd, SOL_SOCKET, SO_PRIORITY, &priority, sizeof(priority));
  //TODO SO_RCVBUF (not < than MTU)
  //TODO SO_BINDTODEVICE

  return sock_fd;
}

Client::~Client(void)
{
  close(tcp_sock_fd);
  close(udp_sock_fd);
}

void Client::Run(void)
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
  //busy polling, blocking socket
  while (true)
  {
    //recvmsg with address specified
    //handle order
    //pass message to RobustnessHandler which checks sequence 
  }
}

COLD bool Client::sendLogin(void)
{
  SoupBinTCPPacket packet{};

  packet.length = sizeof(packet.type) + sizeof(packet.login_request);
  packet.type = 'L';

  auto &login = packet.login_request;

  std::memcpy(login.username, config.username.c_str(), sizeof(login.username));
  std::memcpy(login.password, config.password.c_str(), sizeof(login.password));
  std::memset(login.requested_session, ' ', sizeof(login.requested_session));
  login.requested_sequence_number[0] = '1';

  constexpr uint16_t packet_size = sizeof(packet.length) + sizeof(packet.type) + sizeof(login);
  utils::safe_send(tcp_sock_fd, reinterpret_cast<const char *>(&packet), packet_size);

  return true;
}

COLD bool Client::recvLogin(void)
{
  SoupBinTCPPacket packet;
  constexpr uint16_t header_size = sizeof(packet.length) + sizeof(packet.type);

  utils::safe_recv(tcp_sock_fd, reinterpret_cast<char *>(&packet), header_size);

  switch (packet.type)
  {
    case 'H':
      return recvLogin();
    case 'J':
      utils::throw_exception("Login rejected");
      UNREACHABLE;
    case 'A':
    {
      const uint16_t payload_size = utils::bswap32(packet.length) - sizeof(packet.type);
      utils::safe_recv(tcp_sock_fd, reinterpret_cast<char *>(&packet.login_acceptance), payload_size);
      //TODO set sequence number in RobustnessHandler
      return true;
    }
    default:
      utils::throw_exception("Invalid packet type");
  }

  UNREACHABLE;
}

bool Client::recvSnapshot(void)
{
  SoupBinTCPPacket packet;
  constexpr uint16_t header_size = sizeof(packet.length) + sizeof(packet.type);

  utils::safe_recv(tcp_sock_fd, reinterpret_cast<char *>(&packet), header_size);

  switch (packet.type)
  {
    case 'H':
      return recvSnapshot();
    case 'S':
    {
      const uint16_t payload_size = utils::bswap32(packet.length) - sizeof(packet.type);
      std::vector<char> buffer(payload_size);
      utils::safe_recv(tcp_sock_fd, buffer.data(), payload_size);
      return processMessageBlocks(buffer);
    }
    default:
      utils::throw_exception("Invalid packet type");
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

  utils::safe_send(tcp_sock_fd, reinterpret_cast<const char *>(&packet), packet_size);

  return true;
}

bool Client::processMessageBlocks(const std::vector<char> &buffer)
{
  auto it = buffer.cbegin();
  const auto end = buffer.cend();

  static const std::unordered_map<char, uint16_t> message_sizes = {
    {'T', sizeof(MessageBlock::seconds)},
    {'R', sizeof(MessageBlock::series_info_basic)},
    {'M', sizeof(MessageBlock::series_info_basic_combination)},
    {'L', sizeof(MessageBlock::tick_size_data)},
    {'S', sizeof(MessageBlock::system_event_data)},
    {'O', sizeof(MessageBlock::trading_status_data)},
    {'A', sizeof(MessageBlock::new_order)},
    {'E', sizeof(MessageBlock::execution_notice)},
    {'C', sizeof(MessageBlock::execution_notice_with_trade_info)},
    {'D', sizeof(MessageBlock::deleted_order)},
    {'Z', sizeof(MessageBlock::ep)},
    {'G', sizeof(MessageBlock::snapshot_completion)}
  };

  while (it != end)
  {
    const MessageBlock &block = *reinterpret_cast<const MessageBlock *>(&*it);
    const uint16_t block_length = utils::bswap16(block.length);

    utils::assert(message_sizes.at(block.type) == block_length, "Unexpected message block length");

    switch (block.type)
    {
      case 'A':
        handleNewOrder(block);
        break;
      case 'D':
        handleDeletedOrder(block);
        break;
      case 'G':
        //TODO pass received sequence number to RobustnessHandler
        return true;
      default:
        break;
    }

    std::advance(it, block.length);
  }

  return false;
}

HOT void Client::handleNewOrder(const MessageBlock &block)
{
  const auto &new_order = block.new_order;
  const OrderBook::Side side = static_cast<OrderBook::Side>(new_order.side == 'S');
  const int32_t price = utils::bswap32(new_order.price);
  const uint64_t volume = utils::bswap32(new_order.quantity);

  if (price == INT32_MIN)
    order_book.executeOrder(side, volume);
  else
    order_book.addOrder(side, price, volume);
}

//TODO find a way to efficiently remove orders
HOT void Client::handleDeletedOrder(const MessageBlock &block)
{
  (void)block;
  // const auto &deleted_order = block.deleted_order;
  // const OrderBook::Side side = static_cast<OrderBook::Side>(deleted_order.side == 'S');
  // const uint32_t price = utils::bswap32(deleted_order.price);
  // const uint64_t volume = utils::bswap32(deleted_order.quantity);

  // order_book.removeOrder(side, price, volume);
}