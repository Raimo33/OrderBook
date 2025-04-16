/*================================================================================

File: MessageHandler.inl                                                        
Creator: Claudio Raimondi                                                       
Email: claudio.raimondi@pm.me                                                   

created at: 2025-04-16 17:40:46                                                 
last edited: 2025-04-16 17:40:46                                                

================================================================================*/

#include "MessageHandler.hpp"

void MessageHandler::addBookId(const uint32_t orderbook_id)
{
  monitored_orderbooks.insert(orderbook_id);
}