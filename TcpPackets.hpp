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

struct Data
{
  std::vector<char> message;
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