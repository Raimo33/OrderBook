/*================================================================================

File: MessageHandler.cpp                                                        
Creator: Claudio Raimondi                                                       
Email: claudio.raimondi@pm.me                                                   

created at: 2025-04-05 10:36:57                                                 
last edited: 2025-04-17 19:18:53                                                

================================================================================*/

#include <array>
#include <span>
#include <cstdint>

#include "MessageHandler.hpp"
#include "Packets.hpp"
#include "utils/utils.hpp"
#include "macros.hpp"
#include "error.hpp"

COLD MessageHandler::MessageHandler(void) noexcept
{
}

COLD MessageHandler::~MessageHandler() noexcept
{
}

HOT void MessageHandler::handleMessage(const MessageData &data)
{
  using Handler = void (MessageHandler::*)(const MessageData &);

  static constexpr uint8_t size = 'Z' + 1;
  static constexpr std::array<Handler, size> handlers = []()
  {
    std::array<Handler, size> handlers{};
    handlers['T'] = &MessageHandler::handleSeconds;
    handlers['A'] = &MessageHandler::handleNewOrder;
    handlers['D'] = &MessageHandler::handleDeletedOrder;
    handlers['E'] = &MessageHandler::handleExecutionNotice;
    handlers['C'] = &MessageHandler::handleExecutionNoticeWithTradeInfo;
    handlers['Z'] = &MessageHandler::handleEquilibriumPrice;
    handlers['R'] = &MessageHandler::handleSeriesInfoBasic;
    handlers['M'] = &MessageHandler::handleSeriesInfoBasicCombination;
    handlers['L'] = &MessageHandler::handleTickSizeData;
    handlers['S'] = &MessageHandler::handleSystemEvent;
    handlers['O'] = &MessageHandler::handleTradingStatus;
    return handlers;
  }();

  (this->*handlers[data.type])(data);
}

HOT void MessageHandler::handleNewOrder(const MessageData &data)
{
  using Handler = void (MessageHandler::*)(const MessageData &);
  static constexpr Handler handlers[] = {
    &MessageHandler::handleNewLimitOrder,
    &MessageHandler::handleNewMarketOrder
  };

  const bool is_market = (data.new_order.price == INT32_MIN);
  (this->*handlers[is_market])(data);
}

HOT void MessageHandler::handleDeletedOrder(const MessageData &data)
{
  const uint32_t orderbook_id = data.deleted_order.orderbook_id;

  static constexpr OrderBookOp op = +[](OrderBook *book, const MessageData &data) noexcept
  {
    const auto &m = data.deleted_order;
    const OrderBook::Side side = static_cast<OrderBook::Side>(m.side == 'S');
    book->removeOrder(m.order_id, side);
  };

  processOrderBookOperation<op>(orderbook_id, data);
}

HOT void MessageHandler::handleExecutionNotice(const MessageData &data)
{
  const uint32_t orderbook_id = data.execution_notice.orderbook_id;

  static constexpr OrderBookOp op = +[](OrderBook *book, const MessageData &data) noexcept
  {
    const auto &m = data.execution_notice;
    const OrderBook::Side side = static_cast<OrderBook::Side>(m.side == 'S');
    book->executeOrder(m.order_id, side, m.executed_quantity);
  };

  processOrderBookOperation<op>(orderbook_id, data);
}

HOT void MessageHandler::handleExecutionNoticeWithTradeInfo(const MessageData &data)
{
  const uint32_t orderbook_id = data.execution_notice_with_trade_info.orderbook_id;

  static constexpr OrderBookOp op = +[](OrderBook *book, const MessageData &data) noexcept
  {
    const auto &m = data.execution_notice_with_trade_info;
    const OrderBook::Side side = static_cast<OrderBook::Side>(m.side == 'S');
    book->removeOrder(m.order_id, side, m.trade_price, m.executed_quantity);
  };

  processOrderBookOperation<op>(orderbook_id, data);
}

void MessageHandler::handleEquilibriumPrice(const MessageData &data)
{
  const uint32_t orderbook_id = data.ep.orderbook_id;

  static constexpr OrderBookOp op = +[](OrderBook *book, const MessageData &data) noexcept
  {
    const auto &m = data.ep;
    book->setEquilibrium(m.equilibrium_price, m.available_bid_quantity, m.available_ask_quantity);
  };

  processOrderBookOperation<op>(orderbook_id, data);
}

HOT void MessageHandler::handleSeconds(UNUSED const MessageData &data)
{
}

COLD void MessageHandler::handleSeriesInfoBasic(const MessageData &data)
{
  const uint32_t orderbook_id = data.series_info_basic.orderbook_id;

  if (orderbook_whitelist.contains(orderbook_id) == false)
    return;

  order_books.ids.push_back(orderbook_id);
  order_books.books.emplace_back();
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
  const uint32_t orderbook_id = data.new_order.orderbook_id;

  static constexpr OrderBookOp op = +[](OrderBook *book, const MessageData &data) noexcept
  {
    const auto &m = data.new_order;
    const OrderBook::Side side = static_cast<OrderBook::Side>(m.side == 'S');
    book->addOrder(m.order_id, side, m.price, m.quantity);
  };

  processOrderBookOperation<op>(orderbook_id, data);
}

HOT void MessageHandler::handleNewMarketOrder(UNUSED const MessageData &data)
{
}

template <MessageHandler::OrderBookOp op>
HOT void MessageHandler::processOrderBookOperation(const uint32_t orderbook_id, const MessageData &data)
{
  OrderBook *book = getOrderBook(orderbook_id);
  const uint8_t is_valid = !!book;

  static constexpr OrderBookOp noOp = +[](OrderBook *, const MessageData &) noexcept {};
  static constexpr OrderBookOp handlers[] = {noOp, op};
  handlers[is_valid](book, data);
}

HOT inline OrderBook *MessageHandler::getOrderBook(const uint32_t orderbook_id) noexcept
{
  static constexpr std::equal_to<uint32_t> comp{};
  ssize_t idx = utils::forward_lower_bound(std::span<const uint32_t>{order_books.ids}, orderbook_id, comp);

  const bool found = (idx != -1);
  idx *= found;

  OrderBook *book = &order_books.books[idx];
  return reinterpret_cast<OrderBook *>(found * reinterpret_cast<uintptr_t>(book));
}