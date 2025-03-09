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

int main(int argc, char* argv[])
{
  try
  {
    if (argc != 3)
      throw std::runtime_error("Usage: ./program <IP> <PORT>");

    std::string ip(argv[1]);
    uint16_t port = std::stoi(argv[2]);

    Client client(ip, port);
    client.run();
  }
  catch (const std::exception& e)
  {
    std::cerr << "Error: " << e.what() << std::endl;
    return 1;
  }
}