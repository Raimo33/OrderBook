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

int main(void)
{
  try
  {
    Config config = load_config();
    Client client;
    client.run();
  }
  catch (const std::exception& e)
  {
    std::cerr << "Error: " << e.what() << std::endl;
    return 1;
  }
}