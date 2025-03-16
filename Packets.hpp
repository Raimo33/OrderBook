#pragma once

#include <cstdint>
#include <vector>

#pragma pack(push, 1)

struct SoupBinTCPPacket
{
  uint16_t length;
  char type;

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

  struct SequencedData {};
  struct ServerHeartbeat {};
  struct ClientHeartbeat {};
  struct LogoutRequest {};

  union
  {
    LoginRequest login_request;
    LoginAcceptance login_acceptance;
    LoginReject login_reject;
  } data;
};

struct MoldUDP64Header
{
  char session[10];
  uint16_t message_count;
  uint64_t sequence_number;
};

struct MessageBlock
{
  uint16_t length;
  char type;

  struct SnapshotCompletion
  {
    char sequence[20];
  };

  struct Seconds
  {
    uint32_t second;
  };

  struct SeriesInfoBasic
  {
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
    uint32_t timestamp_nanoseconds;
    uint32_t combination_orderbook_id;
    uint32_t leg_orderbook_id;
    char leg_slide;
    int32_t leg_ratio;
  };

  struct TickSizeData
  {
    uint32_t timestamp_nanoseconds;
    uint32_t orderbook_id;
    uint64_t tick_size;
    uint32_t price_from;
    uint32_t price_to;
  };

  struct SystemEventData
  {
    uint32_t timestamp_nanoseconds;
    char event_code;
  };

  struct TradingStatusData
  {
    uint32_t timestamp_nanoseconds;
    uint32_t orderbook_id;
    char state_name[20];
  };

  struct NewOrder
  {
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
    uint32_t timestamp_nanoseconds;
    uint64_t order_id;
    uint32_t orderbook_id;
    char side;
  };

  struct EP
  {
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

  union
  {
    SnapshotCompletion snapshot_completion;
    Seconds seconds;
    SeriesInfoBasic series_info_basic;
    SeriesInfoBasicCombination series_info_basic_combination;
    TickSizeData tick_size_data;
    SystemEventData system_event_data;
    TradingStatusData trading_status_data;
    NewOrder new_order;
    ExecutionNotice execution_notice;
    ExecutionNoticeWithTradeInfo execution_notice_with_trade_info;
    DeletedOrder deleted_order;
    EP ep;
  } data;
};

#pragma pack(pop)