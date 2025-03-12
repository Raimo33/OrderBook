#pragma once

#include <cstdint>
#include <vector>

//TODO explore the concept of unions (std::variant)
//TODO namespaces to split between tcp bodies and udp bodies

#pragma pack(push, 1)

struct SoupBinTCPHeader
{
  uint16_t length;
  uint8_t type;
};

template <typename T>
struct SoupBinTCPPacket
{
  SoupBinTCPHeader header;
  T body;
};

struct MoldUDP64Header
{
  char session[10];
  uint16_t message_count;
  uint64_t sequence_number;
};

template <typename T>
struct MoldUDP64Packet
{
  MoldUDP64Header header;
  T body;
};

template <typename T>
struct MessageBlock
{
  uint16_t message_length;
  T message_data;
};


struct EmptyBody {};

struct LoginRequest
{
  char username[6];
  char password[10];
  char requested_session[10];
  char requested_sequence_number[20];
};

struct LoginAcceptance
{
  char session[10];
  char sequence_number[20];
};

struct LoginReject
{
  char reject_reason_code;
};

//TODO capire se serve, magari array di union
struct SequencedData
{
  //one message (timestamp, order execution, etc)
  //one message (timestamp, order execution, etc)
  //one message (timestamp, order execution, etc)
};

struct SnapshotCompletion
{
  char message_type;
  char sequence_number[20];
};

using ServerHeartbeat = EmptyBody;
using UserHeartbeat = EmptyBody;
using LogoutRequest = EmptyBody;


#pragma pack(pop)