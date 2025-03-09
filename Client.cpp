/*================================================================================

File: Client.cpp                                                                
Creator: Claudio Raimondi                                                       
Email: claudio.raimondi@pm.me                                                   

created at: 2025-03-08 15:48:16                                                 
last edited: 2025-03-08 21:24:05                                                

================================================================================*/

#include <arpa/inet.h>

#include "Client.hpp"
#include "macros.hpp"

//TODO bind to specific static IP instead of INADDR_ANY

Client::Client(void) :
  tcp_handler(order_book),
  udp_handler(order_book) {}

Client::~Client(void) {}

void Client::run(void)
{
  while (tcp_handler.get_state() != TcpHandler::LOGGED_OUT)
  {
    tcp_handler.request_orderbook();
    udp_handler.accumulate_updates();
  }

  //TODO in parallel
  {
    udp_handler.update_orderbook()
    engine.print_best_prices();
  }
}