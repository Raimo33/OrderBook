#pragma once

#include <string>
#include <array>


struct Config
{

  struct Endpoint
  {
    std::string ip;
    int port;
  };

  std::string bind_address;
  int udp_port;
  int tcp_port;
  std::array<Endpoint, 2> multicast_endpoints;
  std::array<Endpoint, 2> rewind_endpoints;
  std::array<Endpoint, 2> glimpse_endpoints;
  std::string user_id;
  std::string password;
};

Config load_config(void);