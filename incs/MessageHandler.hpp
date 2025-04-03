#pragma once

#include <cstdint>
#include <vector>

#include "OrderBook.hpp"
#include "packets.hpp"

class MessageHandler
{
  public:
    MessageHandler(void) noexcept;
    ~MessageHandler(void) noexcept;

    void handleMessage(const MessageData &data);

  private:

    void handleSnapshotCompletion(const MessageData &data);
    void handleNewOrder(const MessageData &data);
    void handleDeletedOrder(const MessageData &data);
    void handleSeconds(const MessageData &data);
    void handleSeriesInfoBasic(const MessageData &data);
    void handleSeriesInfoBasicCombination(const MessageData &data);
    void handleTickSizeData(const MessageData &data);
    void handleSystemEvent(const MessageData &data);
    void handleTradingStatus(const MessageData &data);
    void handleExecutionNotice(const MessageData &data);
    void handleExecutionNoticeWithTradeInfo(const MessageData &data);
    void handleEquilibriumPrice(const MessageData &data);
    
    void handleNewLimitOrder(const MessageData &data);
    void handleNewMarketOrder(const MessageData &data);

    struct OrderBooks
    {
      std::vector<uint32_t> ids;
      std::vector<OrderBook> books;
    } order_books;
};