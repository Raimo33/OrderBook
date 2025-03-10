/*================================================================================

File: Client.cpp                                                                
Creator: Claudio Raimondi                                                       
Email: claudio.raimondi@pm.me                                                   

created at: 2025-03-08 15:48:16                                                 
last edited: 2025-03-08 21:24:05                                                

================================================================================*/

#include <arpa/inet.h>

#include "Client.hpp"
#include "Config.hpp"
#include "macros.hpp"

Client::Client(const Config &config) :
  config(config),
  server_idx(0),
  tcp_handler(config.client_conf, config.server_confs[server_idx], order_book),
  udp_handler(config.client_conf, config.server_confs[server_idx], order_book) {}

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
    //switch server somewhere here
    engine.print_best_prices();
  }
}

void Client::switch_server(void)
{
  server_idx ^= 1;

  tcp_handler = TcpHandler(config.client_conf, config.server_confs[server_idx], order_book);
  udp_handler = UdpHandler(config.client_conf, config.server_confs[server_idx], order_book);
}