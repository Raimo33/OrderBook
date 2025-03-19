#pragma once

#include <string>

struct Config
{
  Config(void);

  std::string bind_ip;
  std::string bind_port_udp;
  std::string bind_port_tcp;
  std::string multicast_ip;
  std::string multicast_port;
  std::string glimpse_ip;
  std::string glimpse_port;
  std::string rewind_ip;
  std::string rewind_port;
  std::string username;
  std::string password;
};