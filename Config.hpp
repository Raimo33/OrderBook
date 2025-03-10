#pragma once

#include <string>
#include <array>

struct ClientConfig
{
  std::string bind_address;
  int udp_port;
  int tcp_port;
  std::string username;
  std::string password;
};

struct ServerConfig
{
  struct Endpoint
  {
    std::string ip;
    int port;
  };

  Endpoint multicast_endpoint;
  Endpoint rewind_endpoint;
  Endpoint glimpse_endpoint;
};

struct Config
{
  ClientConfig client_conf;
  std::array<ServerConfig, 2> server_confs;
};

Config load_config(void);