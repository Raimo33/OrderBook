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

Client::Client(const std::string_view ip_str, const uint16_t port) :
  addr({.sin_family = AF_INET, .sin_port = htons(port), .sin_addr.s_addr = inet_addr(ip_str.data())}),
  tcp_handler(addr, order_book),
  udp_handler(addr, order_book)

Client::~Client(void) {}

void Client::run(void)
{
  //TODO in parallel
  tcp_handler.request_snapshot();
  udp_handler.receive_updates();
}