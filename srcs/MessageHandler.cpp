/*================================================================================

File: MessageHandler.cpp                                                        
Creator: Claudio Raimondi                                                       
Email: claudio.raimondi@pm.me                                                   

created at: 2025-04-05 10:36:57                                                 
last edited: 2025-04-05 11:07:01                                                

================================================================================*/

#include <array>
#include <span>
#include <cstdint>

#include "MessageHandler.hpp"
#include "packets.hpp"
#include "utils/utils.hpp"
#include "macros.hpp"
#include "error.hpp"

COLD MessageHandler::MessageHandler(void) noexcept :
  order_books()
{
}

COLD MessageHandler::~MessageHandler(void) noexcept
{
}

HOT void MessageHandler::handleMessage(const MessageData &data)
{
  using Handler = void (MessageHandler::*)(const MessageData &);

  static constexpr uint8_t size = 'Z' + 1;
  static constexpr std::array<Handler, size> handlers = []()
  {
    std::array<Handler, size> handlers{};
    handlers['A'] = &MessageHandler::handleNewOrder;
    handlers['D'] = &MessageHandler::handleDeletedOrder;
    handlers['T'] = &MessageHandler::handleSeconds;
    handlers['R'] = &MessageHandler::handleSeriesInfoBasic;
    handlers['M'] = &MessageHandler::handleSeriesInfoBasicCombination;
    handlers['L'] = &MessageHandler::handleTickSizeData;
    handlers['S'] = &MessageHandler::handleSystemEvent;
    handlers['O'] = &MessageHandler::handleTradingStatus;
    handlers['E'] = &MessageHandler::handleExecutionNotice;
    handlers['C'] = &MessageHandler::handleExecutionNoticeWithTradeInfo;
    handlers['Z'] = &MessageHandler::handleEquilibriumPrice;
    return handlers;
  }();

  (this->*handlers[data.type])(data);
}

HOT void MessageHandler::handleNewOrder(const MessageData &data)
{
  const auto &new_order = data.new_order;
  const int32_t price = utils::to_host(new_order.price);

  using Handler = void (MessageHandler::*)(const MessageData &);
  static constexpr Handler handlers[2] = {
    &MessageHandler::handleNewLimitOrder,
    &MessageHandler::handleNewMarketOrder
  };

  const bool is_market = (price == INT32_MIN);
  (this->*handlers[is_market])(data);
}

HOT void MessageHandler::handleDeletedOrder(const MessageData &data)
{
  const auto &deleted_order = data.deleted_order;
  const uint32_t book_id = utils::to_host(deleted_order.orderbook_id);
  const uint64_t order_id = utils::to_host(deleted_order.order_id);
  const OrderBook::Side side = static_cast<OrderBook::Side>(deleted_order.side);

  static constexpr std::equal_to<uint32_t> comp{};
  const size_t idx = utils::find(std::span<const uint32_t>(order_books.ids), book_id, comp);
  order_books.books[idx].removeOrder(order_id, side);
}

HOT void MessageHandler::handleExecutionNotice(const MessageData &data)
{
  const auto &execution_notice = data.execution_notice;
  const uint32_t book_id = utils::to_host(execution_notice.orderbook_id);
  const uint64_t order_id = utils::to_host(execution_notice.order_id);
  const OrderBook::Side resting_side = static_cast<OrderBook::Side>(execution_notice.side == 'S');
  const uint64_t qty = utils::to_host(execution_notice.executed_quantity);

  static constexpr std::equal_to<uint32_t> comp{};
  const size_t idx = utils::find(std::span<const uint32_t>(order_books.ids), book_id, comp);
  order_books.books[idx].executeOrder(order_id, resting_side, qty);
}

HOT void MessageHandler::handleExecutionNoticeWithTradeInfo(const MessageData &data)
{
  const auto &execution_notice = data.execution_notice_with_trade_info;
  const uint32_t book_id = utils::to_host(execution_notice.orderbook_id);
  const uint64_t order_id = utils::to_host(execution_notice.order_id);
  const OrderBook::Side resting_side = static_cast<OrderBook::Side>(execution_notice.side == 'S');
  const uint64_t qty = utils::to_host(execution_notice.executed_quantity);
  const int32_t price = utils::to_host(execution_notice.trade_price);

  static constexpr std::equal_to<uint32_t> comp{};
  const size_t idx = utils::find(std::span<const uint32_t>(order_books.ids), book_id, comp);
  order_books.books[idx].removeOrder(order_id, resting_side, price, qty);
}

COLD void MessageHandler::handleEquilibriumPrice(const MessageData &data)
{
  const auto &equilibrium_price = data.ep;
  const uint32_t book_id = utils::to_host(equilibrium_price.orderbook_id);
  const int32_t price = utils::to_host(equilibrium_price.equilibrium_price);
  const uint64_t bid_qty = utils::to_host(equilibrium_price.available_bid_quantity);
  const uint64_t ask_qty = utils::to_host(equilibrium_price.available_ask_quantity);

  static constexpr std::equal_to<uint32_t> comp{};
  const size_t idx = utils::find(std::span<const uint32_t>(order_books.ids), book_id, comp);
  order_books.books[idx].setEquilibrium(price, bid_qty, ask_qty);
}

HOT void MessageHandler::handleSeconds(UNUSED const MessageData &data)
{
}

COLD void MessageHandler::handleSeriesInfoBasic(const MessageData &data)
{
  const auto &series_info_basic = data.series_info_basic;
  const uint32_t book_id = utils::to_host(series_info_basic.orderbook_id);

  order_books.ids.push_back(book_id);
  order_books.books.emplace_back(OrderBook());
}

COLD void MessageHandler::handleSeriesInfoBasicCombination(UNUSED const MessageData &data)
{
}

COLD void MessageHandler::handleTickSizeData(UNUSED const MessageData &data)
{
}

COLD void MessageHandler::handleSystemEvent(UNUSED const MessageData &data)
{
}

COLD void MessageHandler::handleTradingStatus(UNUSED const MessageData &data)
{
  //"M_ZARABA", "A_ZARABA_E", "A_ZARABA_E2", "N_ZARABA", "A_ZARABA"
  // if (status[2] == 'Z')
  //   resumeTrading();
}

HOT void MessageHandler::handleNewLimitOrder(const MessageData &data)
{
  const auto &new_order = data.new_order;
  const int32_t price = utils::to_host(new_order.price);
  const uint32_t book_id = utils::to_host(new_order.orderbook_id);
  const uint64_t order_id = utils::to_host(new_order.order_id);
  const OrderBook::Side side = static_cast<OrderBook::Side>(new_order.side);
  const uint64_t qty = utils::to_host(new_order.quantity);

  static constexpr std::equal_to<uint32_t> comp{};
  const size_t idx = utils::find(std::span<const uint32_t>(order_books.ids), book_id, comp);
  order_books.books[idx].addOrder(order_id, side, price, qty);
}

HOT void MessageHandler::handleNewMarketOrder(UNUSED const MessageData &data)
{
}