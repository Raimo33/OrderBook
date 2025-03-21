/*================================================================================

File: main.cpp                                                                  
Creator: Claudio Raimondi                                                       
Email: claudio.raimondi@pm.me                                                   

created at: 2025-03-08 18:21:38                                                 
last edited: 2025-03-08 21:24:05                                                

================================================================================*/

#include <csignal>

#include "Client.hpp"
#include "Config.hpp"

extern volatile bool error;

//TODO disaster recovery
//TODO glimpse
//TODO rewind

void init_signal_handler(void);

int main(void)
{
  error = false;

  init_signal_handler();

  Client client;
  client.run();
}

void init_signal_handler(void)
{
  struct sigaction sa{};
  sa.sa_handler = [](int) { error = true; };

  error |= (sigaction(SIGINT, &sa, nullptr) == -1);
  error |= (sigaction(SIGTERM, &sa, nullptr) == -1);
  error |= (sigaction(SIGQUIT, &sa, nullptr) == -1);
  error |= (sigaction(SIGPIPE, &sa, nullptr) == -1);
}