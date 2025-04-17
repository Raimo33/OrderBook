/*================================================================================

File: Packets.hpp                                                               
Creator: Claudio Raimondi                                                       
Email: claudio.raimondi@pm.me                                                   

created at: 2025-03-23 22:29:52                                                 
last edited: 2025-04-17 16:57:31                                                

================================================================================*/

#pragma once

#include <boost/endian/arithmetic.hpp>

using namespace boost::endian;

#pragma pack(push, 1)

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
      big_uint32_t second;
    } seconds;

    struct
    {
      big_uint32_t timestamp_nanoseconds;
      big_uint32_t orderbook_id;
      char symbol[32];
      char long_name[32];
      char reserved[12];
      uint8_t financial_product;
      char trading_currency[3];
      big_uint16_t number_of_decimals_in_price;
      big_uint16_t number_of_decimals_in_nominal_value;
      big_uint32_t odd_lot_size;
      big_uint32_t round_lot_size;
      big_uint32_t block_lot_size;
      big_uint64_t nominal_value;
      uint8_t number_of_legs;
      big_uint32_t underlying_orderbook_id;
      big_int32_t strike_price;
      big_int32_t expiration_date;
      big_uint16_t number_of_decimals_in_strike_price;
      uint8_t put_or_call;
    } series_info_basic;

    struct
    {
      big_uint32_t timestamp_nanoseconds;
      big_uint32_t combination_orderbook_id;
      big_uint32_t leg_orderbook_id;
      char leg_slide;
      big_int32_t leg_ratio;
    } series_info_basic_combination;

    struct
    {
      big_uint32_t timestamp_nanoseconds;
      big_uint32_t orderbook_id;
      big_uint64_t tick_size;
      big_uint32_t price_from;
      big_uint32_t price_to;
    } tick_size_data;

    struct
    {
      big_uint32_t timestamp_nanoseconds;
      char event_code;
    } system_event;

    struct
    {
      big_uint32_t timestamp_nanoseconds;
      big_uint32_t orderbook_id;
      char state_name[20];
    } trading_status;

    struct
    {
      big_uint32_t timestamp_nanoseconds;
      big_uint64_t order_id;
      big_uint32_t orderbook_id;
      char side;
      big_uint32_t orderbook_position;
      big_uint64_t quantity;
      big_int32_t price;
      big_uint16_t order_attributes;
      uint8_t lot_type;
    } new_order;

    struct
    {
      big_uint32_t timestamp_nanoseconds;
      big_uint64_t order_id;
      big_uint32_t orderbook_id;
      char side;
      big_uint64_t executed_quantity;
      big_uint64_t match_id;
      big_uint32_t combo_group_id;
      char reserved1[7];
      char reserved2[7];
    } execution_notice;

    struct
    {
      big_uint32_t timestamp_nanoseconds;
      big_uint64_t order_id;
      big_uint32_t orderbook_id;
      char side;
      big_uint64_t executed_quantity;
      big_uint64_t match_id;
      big_uint32_t combo_group_id;
      char reserved1[7];
      char reserved2[7];
      big_uint32_t trade_price;
      char occurred_cross;
      char printable;
    } execution_notice_with_trade_info;

    struct
    {
      big_uint32_t timestamp_nanoseconds;
      big_uint64_t order_id;
      big_uint32_t orderbook_id;
      char side;
    } deleted_order;

    struct
    {
      big_uint32_t timestamp_nanoseconds;
      big_uint32_t orderbook_id;
      big_uint64_t available_bid_quantity;
      big_uint64_t available_ask_quantity;
      big_uint32_t equilibrium_price;
      char reserved1[4];
      char reserved2[4];
      char reserved3[8];
      char reserved4[8];
    } ep;
  };
};

struct SoupBinTCPPacket
{
  big_uint16_t body_length;

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
  big_uint64_t sequence_number;
  big_uint16_t message_count;
};

struct MessageBlock
{
  big_uint16_t length;
  MessageData data;
};

#pragma pack(pop)

