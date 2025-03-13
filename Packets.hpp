#pragma once

#include <cstdint>
#include <vector>

#pragma pack(push, 1)

struct SoupBinTCPHeader
{
  uint16_t length;
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
struct MessageBlock
{
  uint16_t message_length;
  T message_data;
};

template <typename T>
struct MoldUDP64Packet
{
  MoldUDP64Header header;
  T body;
};

namespace soupbintcp_body
{
  struct LoginRequest
  {
    uint8_t type;
    char username[6];
    char password[10];
    char requested_session[10];
    char requested_sequence_number[20];
  };

  struct LoginAcceptance
  {
    uint8_t type;
    char session[10];
    char sequence_number[20];
  };

  struct LoginReject
  {
    uint8_t type;
    char reject_reason_code;
  };

  struct SnapshotData
  {
    uint8_t type;
    //a series of 'message data', terminates with the message_data snapshot completion
  };

  struct ServerHeartbeat
  {
    uint8_t type;
  };

  struct UserHeartbeat
  {
    uint8_t type;
  };

  struct LogoutRequest
  {
    uint8_t type;
  };
}

struct LoginRequest
{
  uint8_t type;
  char username[6];
  char password[10];
  char requested_session[10];
  char requested_sequence_number[20];
};

struct LoginAcceptance
{
  uint8_t type;
  char session[10];
  char sequence_number[20];
};

struct LoginReject
{
  uint8_t type;
  char reject_reason_code;
};

struct SnapshotData
{
  uint8_t type;
  //a series of 'message data', terminates with the message_data snapshot completion
};

struct ServerHeartbeat
{
  uint8_t type;
};

struct UserHeartbeat
{
  uint8_t type;
};

struct LogoutRequest
{
  uint8_t type;
};


namespace message_data
{

  struct HeartBeat {};

  struct SnapshotCompletion
  {
    char message_type;
    char sequence[20];
  };

  struct Seconds
  {
    char message_type;
    uint32_t second;
  };

  struct SeriesInfoBasic
  {
    char message_type;
    uint32_t timestamp_nanoseconds;
    uint32_t orderbook_id;
    char symbol[32];
    char long_name[32];
    char reserved[12];
    uint8_t financial_product;
    char trading_currency[3];
    uint16_t number_of_decimals_in_price;
    uint16_t number_of_decimals_in_nominal_value;
    uint32_t odd_lot_size;
    uint32_t round_lot_size;
    uint32_t block_lot_size;
    uint64_t nominal_value;
    uint8_t number_of_legs;
    uint32_t underlying_orderbook_id;
    int32_t strike_price;
    int32_t day;
    uint16_t number_of_decimals_in_strike_price;
    uint8_t put_or_call;
  };

  struct SeriesInfoBasicCombination
  {
    char message_type;
    uint32_t timestamp_nanoseconds;
    uint32_t combination_orderbook_id;
    uint32_t leg_orderbook_id;
    char leg_slide;
    int32_t leg_ratio;
  };

  struct TickSizeData
  {
    char message_type;
    uint32_t timestamp_nanoseconds;
    uint32_t orderbook_id;
    uint64_t tick_size;
    uint32_t price_from;
    uint32_t price_to;
  };

  struct SystemEventdata
  {
    char message_type;
    uint32_t timestamp_nanoseconds;
    char event_code;
  };

  struct TradingStatusData
  {
    char message_type;
    uint32_t timestamp_nanoseconds;
    uint32_t orderbook_id;
    char state_name[20];
  };

  struct NewOrder
  {
    char message_type;
    uint32_t timestamp_nanoseconds;
    uint64_t order_id;
    uint32_t orderbook_id;
    char side;
    uint32_t orderbook_position;
    uint64_t quantity;
    int32_t price;
    uint16_t order_attributes;
    uint8_t lot_type;
  };

  struct ExecutionNotice
  {
    char message_type;
    uint32_t timestamp_nanoseconds;
    uint64_t order_id;
    uint32_t orderbook_id;
    char side;
    uint64_t executed_quantity;
    uint64_t match_id;
    uint32_t combo_group_id;
    char reserved1[7];
    char reserved2[7];
  };

  struct ExecutionNoticeWithTradeInfo
  {
    char message_type;
    uint32_t timestamp_nanoseconds;
    uint64_t order_id;
    uint32_t orderbook_id;
    char side;
    uint64_t executed_quantity;
    uint64_t match_id;
    uint32_t combo_group_id;
    char reserved1[7];
    char reserved2[7];
    uint32_t trade_price;
    char occurred_cross;
    char printable;
  };

  struct DeletedOrder
  {
    char message_type;
    uint32_t timestamp_nanoseconds;
    uint64_t order_id;
    uint32_t orderbook_id;
    char side;
  };

  struct EP
  {
    char message_type;
    uint32_t timestamp_nanoseconds;
    uint32_t orderbook_id;
    uint64_t available_bid_quantity;
    uint64_t available_ask_quantity;
    uint32_t equilibrium_price;
    char reserved1[4];
    char reserved2[4];
    char reserved3[8];
    char reserved4[8];
  };

}


#pragma pack(pop)