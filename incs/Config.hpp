/*================================================================================

File: config.hpp                                                                
Creator: Claudio Raimondi                                                       
Email: claudio.raimondi@pm.me                                                   

created at: 2025-03-30 15:01:52                                                 
last edited: 2025-04-05 14:48:44                                                

================================================================================*/

#pragma once

#include <string>

#define MTU 1500
#define SOCK_BUFSIZE 8388608
#define MAX_BURST_PACKETS 32

//TODO are there some ports faster than others?

struct Config
{
  std::string bind_ip;
  std::string multicast_ip;
  std::string multicast_port;

  std::string rewind_ip;
  std::string rewind_port;

  std::string glimpse_ip;
  std::string glimpse_port;

  std::string username;
  std::string password;
};