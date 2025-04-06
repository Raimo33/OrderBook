/*================================================================================

File: MessageHandler.cpp                                                        
Creator: Claudio Raimondi                                                       
Email: claudio.raimondi@pm.me                                                   

created at: 2025-04-05 10:36:57                                                 
last edited: 2025-04-06 22:29:03                                                

================================================================================*/

#include <array>
#include <span>
#include <cstdint>

#include "MessageHandler.hpp"
#include "Packets.hpp"
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

  using Handler = void (MessageHandler::*)(const MessageData &);
  static constexpr Handler handlers[2] = {
    &MessageHandler::handleNewLimitOrder,
    &MessageHandler::handleNewMarketOrder
  };

  const bool is_market = (new_order.price == INT32_MIN);
  (this->*handlers[is_market])(data);
}

HOT void MessageHandler::handleDeletedOrder(const MessageData &data)
{
  const auto &deleted_order = data.deleted_order;
  const OrderBook::Side side = static_cast<OrderBook::Side>(deleted_order.side == 'S');

  auto &order_book = getOrderBook(deleted_order.orderbook_id);
  order_book.removeOrder(deleted_order.order_id, side);
}

HOT void MessageHandler::handleExecutionNotice(const MessageData &data)
{
  const auto &execution = data.execution_notice;
  const OrderBook::Side resting_side = static_cast<OrderBook::Side>(execution.side == 'S');

  auto &order_book = getOrderBook(execution.orderbook_id);
  order_book.executeOrder(execution.order_id, resting_side, execution.executed_quantity);
}

HOT void MessageHandler::handleExecutionNoticeWithTradeInfo(const MessageData &data)
{
  const auto &execution = data.execution_notice_with_trade_info;
  const OrderBook::Side resting_side = static_cast<OrderBook::Side>(execution.side == 'S');

  auto &order_book = getOrderBook(execution.orderbook_id);
  order_book.removeOrder(execution.order_id, resting_side, execution.trade_price, execution.executed_quantity);
}

void MessageHandler::handleEquilibriumPrice(const MessageData &data)
{
  const auto &ep = data.ep;

  auto &order_book = getOrderBook(ep.orderbook_id);
  order_book.setEquilibrium(ep.equilibrium_price, ep.available_bid_quantity, ep.available_ask_quantity);
}

HOT void MessageHandler::handleSeconds(UNUSED const MessageData &data)
{
}

COLD void MessageHandler::handleSeriesInfoBasic(const MessageData &data)
{
  const auto &series_info_basic = data.series_info_basic;

  order_books.ids.push_back(series_info_basic.orderbook_id);
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
  const OrderBook::Side side = static_cast<OrderBook::Side>(new_order.side);

  auto &order_book = getOrderBook(new_order.orderbook_id);
  order_book.addOrder(new_order.order_id, side, new_order.price, new_order.quantity);
}

HOT void MessageHandler::handleNewMarketOrder(UNUSED const MessageData &data)
{
}

HOT inline OrderBook &MessageHandler::getOrderBook(const uint32_t orderbook_id) noexcept
{
  static constexpr std::equal_to<uint32_t> comp{};
  const size_t idx = utils::find(std::span<const uint32_t>(order_books.ids), orderbook_id, comp);
  return order_books.books[idx];
}