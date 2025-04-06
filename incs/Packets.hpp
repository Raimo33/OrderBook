/*================================================================================

File: Packets.hpp                                                               
Creator: Claudio Raimondi                                                       
Email: claudio.raimondi@pm.me                                                   

created at: 2025-03-23 22:29:52                                                 
last edited: 2025-04-06 18:55:50                                                

================================================================================*/

#pragma once

#include <cstdint>

#pragma pack(push, 1)

template <typename T>
struct NetworkValue
{
  T value;

  explicit NetworkValue(const T network_value = {}) noexcept;

  constexpr inline operator T() const noexcept;
  constexpr inline NetworkValue<T> &operator=(const T &host_value) noexcept;
  constexpr inline T network_order(void) const noexcept;
};

#include "Packets.tpp"

struct MessageData
{
  char type;

  union
  {
    struct
    {
      char sequence[20];
    } snapshot_completion;
  
    struct
    {
      NetworkValue<uint32_t> second;
    } seconds;

    struct
    {
      NetworkValue<uint32_t> timestamp_nanoseconds;
      NetworkValue<uint32_t> orderbook_id;
      char symbol[32];
      char long_name[32];
      char reserved[12];
      uint8_t financial_product;
      char trading_currency[3];
      NetworkValue<uint16_t> number_of_decimals_in_price;
      NetworkValue<uint16_t> number_of_decimals_in_nominal_value;
      NetworkValue<uint32_t> odd_lot_size;
      NetworkValue<uint32_t> round_lot_size;
      NetworkValue<uint32_t> block_lot_size;
      NetworkValue<uint64_t> nominal_value;
      uint8_t number_of_legs;
      NetworkValue<uint32_t> underlying_orderbook_id;
      NetworkValue<int32_t> strike_price;
      NetworkValue<int32_t> expiration_date;
      NetworkValue<uint16_t> number_of_decimals_in_strike_price;
      uint8_t put_or_call;
    } series_info_basic;

    struct
    {
      NetworkValue<uint32_t> timestamp_nanoseconds;
      NetworkValue<uint32_t> combination_orderbook_id;
      NetworkValue<uint32_t> leg_orderbook_id;
      char leg_slide;
      NetworkValue<int32_t> leg_ratio;
    } series_info_basic_combination;

    struct
    {
      NetworkValue<uint32_t> timestamp_nanoseconds;
      NetworkValue<uint32_t> orderbook_id;
      NetworkValue<uint64_t> tick_size;
      NetworkValue<uint32_t> price_from;
      NetworkValue<uint32_t> price_to;
    } tick_size_data;

    struct
    {
      NetworkValue<uint32_t> timestamp_nanoseconds;
      char event_code;
    } system_event;

    struct
    {
      NetworkValue<uint32_t> timestamp_nanoseconds;
      NetworkValue<uint32_t> orderbook_id;
      char state_name[20];
    } trading_status;

    struct
    {
      NetworkValue<uint32_t> timestamp_nanoseconds;
      NetworkValue<uint64_t> order_id;
      NetworkValue<uint32_t> orderbook_id;
      char side;
      NetworkValue<uint32_t> orderbook_position;
      NetworkValue<uint64_t> quantity;
      NetworkValue<int32_t> price;
      NetworkValue<uint16_t> order_attributes;
      uint8_t lot_type;
    } new_order;

    struct
    {
      NetworkValue<uint32_t> timestamp_nanoseconds;
      NetworkValue<uint64_t> order_id;
      NetworkValue<uint32_t> orderbook_id;
      char side;
      NetworkValue<uint64_t> executed_quantity;
      NetworkValue<uint64_t> match_id;
      NetworkValue<uint32_t> combo_group_id;
      char reserved1[7];
      char reserved2[7];
    } execution_notice;

    struct
    {
      NetworkValue<uint32_t> timestamp_nanoseconds;
      NetworkValue<uint64_t> order_id;
      NetworkValue<uint32_t> orderbook_id;
      char side;
      NetworkValue<uint64_t> executed_quantity;
      NetworkValue<uint64_t> match_id;
      NetworkValue<uint32_t> combo_group_id;
      char reserved1[7];
      char reserved2[7];
      NetworkValue<uint32_t> trade_price;
      char occurred_cross;
      char printable;
    } execution_notice_with_trade_info;

    struct
    {
      NetworkValue<uint32_t> timestamp_nanoseconds;
      NetworkValue<uint64_t> order_id;
      NetworkValue<uint32_t> orderbook_id;
      char side;
    } deleted_order;

    struct
    {
      NetworkValue<uint32_t> timestamp_nanoseconds;
      NetworkValue<uint32_t> orderbook_id;
      NetworkValue<uint64_t> available_bid_quantity;
      NetworkValue<uint64_t> available_ask_quantity;
      NetworkValue<uint32_t> equilibrium_price;
      char reserved1[4];
      char reserved2[4];
      char reserved3[8];
      char reserved4[8];
    } ep;
  };
};

struct SoupBinTCPPacket
{
  NetworkValue<uint16_t> body_length;

  struct
  {
    char type;

    union
    {
      struct
      {
        char username[6];
        char password[10];
        char requested_session[10];
        char requested_sequence[20];
      } login_request;

      struct
      {
        char session[10];
        char sequence[20];
      } login_acceptance;

      struct
      {
        char reject_reason_code;
      } login_reject;

      struct {} sequenced_data;
      struct {} server_heartbeat;
      struct {} client_heartbeat;
      struct {} logout_request;
    };
  } body;
};

struct MoldUDP64Header
{
  char session[10];
  NetworkValue<uint64_t> sequence_number;
  NetworkValue<uint16_t> message_count;
};

struct MessageBlock
{
  NetworkValue<uint16_t> length;
  MessageData data;
};

#pragma pack(pop)

