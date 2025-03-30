/*================================================================================

File: main.cpp                                                                  
Creator: Claudio Raimondi                                                       
Email: claudio.raimondi@pm.me                                                   

created at: 2025-03-08 18:21:38                                                 
last edited: 2025-03-30 15:01:52                                                

================================================================================*/

#include <csignal>

#include "Client.hpp"
#include "error.hpp"

volatile bool error = false;

//TODO disaster recovery
//TODO glimpse
//TODO rewind

void init_signal_handler(void);

int main(void)
{
  init_signal_handler();

  Client client;
  client.run();
}

void init_signal_handler(void)
{
  struct sigaction sa{};

  sa.sa_handler = [](int) { panic(); };

  error |= sigaction(SIGINT, &sa, nullptr) == -1;
  error |= sigaction(SIGTERM, &sa, nullptr) == -1;
  error |= sigaction(SIGQUIT, &sa, nullptr) == -1;
  error |= sigaction(SIGPIPE, &sa, nullptr) == -1;

  CHECK_ERROR;
}