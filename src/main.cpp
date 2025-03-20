/*================================================================================

File: main.cpp                                                                  
Creator: Claudio Raimondi                                                       
Email: claudio.raimondi@pm.me                                                   

created at: 2025-03-08 18:21:38                                                 
last edited: 2025-03-08 21:24:05                                                

================================================================================*/

#include <iostream>
#include <stdexcept>

#include "Client.hpp"
#include "Config.hpp"

extern bool error;

int main(void)
{
  error = false;

  Client client;
  client.run();
}