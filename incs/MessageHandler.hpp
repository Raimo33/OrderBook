/*================================================================================

File: MessageHandler.hpp                                                        
Creator: Claudio Raimondi                                                       
Email: claudio.raimondi@pm.me                                                   

created at: 2025-04-06 18:55:50                                                 
last edited: 2025-04-17 16:05:19                                                

================================================================================*/

#pragma once

#include <cstdint>
#include <vector>
#include <unordered_set>

#include "OrderBook.hpp"
#include "Packets.hpp"

class MessageHandler
{
  public:
    MessageHandler(void) noexcept;
    ~MessageHandler() noexcept;

    inline void addBookId(const uint32_t orderbook_id);
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

    using OrderBookOp = void (*)(OrderBook *book, const MessageData &data);
    template <OrderBookOp op>
    void processOrderBookOperation(const uint32_t orderbook_id, const MessageData &data);

    OrderBook *getOrderBook(const uint32_t orderbook_id) noexcept;

    struct OrderBooks
    {
      std::vector<uint32_t> ids;
      std::vector<OrderBook> books;
    } order_books;

    std::unordered_set<uint32_t> orderbook_whitelist;
};

#include "MessageHandler.inl"