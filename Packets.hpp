#pragma once

#include <cstdint>
#include <vector>

#pragma pack(push, 1)

template <typename T>
struct SoupBinTCPPacket
{
  uint16_t length;
  uint8_t type;
  T body;
};

template <typename T>
struct MoldUDP64Packet
{
  //TODO
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

struct SequencedData
{
  //message length
  //message timestamp (optional)
  //one message (timestamp, order execution, etc)
  //message length
  //message timestamp (optional)
  //one message (timestamp, order execution, etc)
  //message length
  //message timestamp (optional)
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