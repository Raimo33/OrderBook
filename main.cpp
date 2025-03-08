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

int main(void)
{
  try
  {
    Client client("239.194.41.1", 21001);
    client.run();
  }
  catch (const std::exception& e)
  {
    std::cerr << e.what() << std::endl;
  }
}