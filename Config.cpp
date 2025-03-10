#include "Config.hpp"

static Config::Endpoint parse_ip_and_port(const std::string& ip_and_port)
{
  const size_t colon_pos = ip_and_port.find(':');
  return {
    .ip = ip_and_port.substr(0, colon_pos),
    .port = std::stoi(ip_and_port.substr(colon_pos + 1))
  };
}

Config load_config(void)
{
  Config config;

  config.bind_address = std::getenv("BIND_ADDR");
  config.udp_port = std::stoi(std::getenv("UDP_PORT"));
  config.tcp_port = std::stoi(std::getenv("TCP_PORT"));

  config.multicast_endpoints = {
    { parse_ip_and_port(std::getenv("MULTICAST_ENDPOINT_1")) },
    { parse_ip_and_port(std::getenv("MULTICAST_ENDPOINT_2")) }
  };

  config.rewind_endpoints = {
    { parse_ip_and_port(std::getenv("REWIND_ENDPOINT_1")) },
    { parse_ip_and_port(std::getenv("REWIND_ENDPOINT_2")) }
  };

  config.glimpse_endpoints = {
    { parse_ip_and_port(std::getenv("GLIMPSE_ENDPOINT_1")) },
    { parse_ip_and_port(std::getenv("GLIMPSE_ENDPOINT_2")) }
  };

  config.user_id = std::getenv("USER_ID");
  config.password = std::getenv("PASSWORD");
}
