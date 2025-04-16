/*================================================================================

File: MessageHandler.cpp                                                        
Creator: Claudio Raimondi                                                       
Email: claudio.raimondi@pm.me                                                   

created at: 2025-04-05 10:36:57                                                 
last edited: 2025-04-16 19:38:28                                                

================================================================================*/

#include <array>
#include <span>
#include <cstdint>

#include "MessageHandler.hpp"
#include "Packets.hpp"
#include "utils/utils.hpp"
#include "macros.hpp"
#include "error.hpp"

using OrderBookOp = void (*)(OrderBook *, const MessageData &);
static constexpr OrderBookOp noOp = +[](OrderBook *, const MessageData &) noexcept {};

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
  const uint32_t orderbook_id = utils::to_host(data.deleted_order.orderbook_id);

  OrderBook *book = getOrderBook(orderbook_id);
  const uint8_t is_valid = !!book;

  static constexpr OrderBookOp removeOrder = +[](OrderBook *book, const MessageData &data) noexcept
  {
    const auto &deleted_order = data.deleted_order;

    const uint64_t order_id = utils::to_host(deleted_order.order_id);
    const OrderBook::Side side = static_cast<OrderBook::Side>(deleted_order.side == 'S');

    book->removeOrder(order_id, side);
  };

  static constexpr OrderBookOp handlers[] = {noOp, removeOrder};
  handlers[is_valid](book, data);
}

HOT void MessageHandler::handleExecutionNotice(const MessageData &data)
{
  const uint32_t orderbook_id = utils::to_host(data.execution_notice.orderbook_id);

  OrderBook *book = getOrderBook(orderbook_id);
  const uint8_t is_valid = !!book;

  static constexpr OrderBookOp executeOrder = +[](OrderBook *book, const MessageData &data) noexcept
  {
    const auto &execution = data.execution_notice;

    const uint64_t order_id = utils::to_host(execution.order_id);
    const OrderBook::Side resting_side = static_cast<OrderBook::Side>(execution.side == 'S');
    const uint64_t executed_quantity = utils::to_host(execution.executed_quantity);

    book->executeOrder(order_id, resting_side, executed_quantity);
  };

  static constexpr OrderBookOp handlers[] = {noOp, executeOrder};
  handlers[is_valid](book, data);
}

HOT void MessageHandler::handleExecutionNoticeWithTradeInfo(const MessageData &data)
{
  const uint32_t orderbook_id = utils::to_host(data.execution_notice_with_trade_info.orderbook_id);

  OrderBook *book = getOrderBook(orderbook_id);
  const uint8_t is_valid = !!book;

  static constexpr OrderBookOp removeOrder = +[](OrderBook *book, const MessageData &data) noexcept
  {
    const auto &execution = data.execution_notice_with_trade_info;

    const uint64_t order_id = utils::to_host(execution.order_id);
    const OrderBook::Side resting_side = static_cast<OrderBook::Side>(execution.side == 'S');
    const uint32_t trade_price = utils::to_host(execution.trade_price);
    const uint64_t executed_quantity = utils::to_host(execution.executed_quantity);

    book->removeOrder(order_id, resting_side, trade_price, executed_quantity);
  };

  static constexpr OrderBookOp handlers[] = {noOp, removeOrder};
  handlers[is_valid](book, data);
}

void MessageHandler::handleEquilibriumPrice(const MessageData &data)
{
  const uint32_t orderbook_id = utils::to_host(data.ep.orderbook_id);

  OrderBook *book = getOrderBook(orderbook_id);
  const uint8_t is_valid = !!book;

  static constexpr OrderBookOp setEquilibriumPrice = +[](OrderBook *book, const MessageData &data) noexcept
  {
    const auto &ep = data.ep;

    const uint32_t equilibrium_price = utils::to_host(ep.equilibrium_price);
    const uint64_t bid_quantity = utils::to_host(ep.available_bid_quantity);
    const uint64_t ask_quantity = utils::to_host(ep.available_ask_quantity);

    book->setEquilibrium(equilibrium_price, bid_quantity, ask_quantity);
  };

  static constexpr OrderBookOp handlers[] = {noOp, setEquilibriumPrice};
  handlers[is_valid](book, data);
}

HOT void MessageHandler::handleSeconds(UNUSED const MessageData &data)
{
}

COLD void MessageHandler::handleSeriesInfoBasic(const MessageData &data)
{
  const auto &series_info_basic = data.series_info_basic;
  const uint32_t orderbook_id = utils::to_host(series_info_basic.orderbook_id);

  if (monitored_orderbooks.contains(orderbook_id) == false)
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
  const uint32_t orderbook_id = utils::to_host(data.new_order.orderbook_id);

  OrderBook *book = getOrderBook(orderbook_id);
  const uint8_t is_valid = !!book;

  static constexpr OrderBookOp addOrder = +[](OrderBook *book, const MessageData &data) noexcept
  {
    const auto &new_order = data.new_order;

    const uint64_t order_id = utils::to_host(new_order.order_id);
    const OrderBook::Side side = static_cast<OrderBook::Side>(new_order.side == 'S');
    const int32_t price = utils::to_host(new_order.price);
    const uint64_t quantity = utils::to_host(new_order.quantity);

    book->addOrder(order_id, side, price, quantity);
  };

  static constexpr OrderBookOp handlers[] = {noOp, addOrder};
  handlers[is_valid](book, data);
}

HOT void MessageHandler::handleNewMarketOrder(UNUSED const MessageData &data)
{
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