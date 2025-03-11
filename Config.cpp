#include "Config.hpp"

static ServerConfig::Endpoint parse_ip_and_port(const std::string &ip_and_port)
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

  config.client_conf = {
    .bind_address = getenv("BIND_ADDRESS"),
    .udp_port = std::stoi(getenv("UDP_PORT")),
    .tcp_port = std::stoi(getenv("TCP_PORT")),
    .username = getenv("USERNAME"),
    .password = getenv("PASSWORD")
  };

  config.server_confs[0] = {
    .multicast_endpoint = parse_ip_and_port(getenv("MULTICAST_ENDPOINT_1")),
    .rewind_endpoint = parse_ip_and_port(getenv("REWIND_ENDPOINT_1")),
    .glimpse_endpoint = parse_ip_and_port(getenv("GLIMPSE_ENDPOINT_1"))
  };

  config.server_confs[1] = {
    .multicast_endpoint = parse_ip_and_port(getenv("MULTICAST_ENDPOINT_2")),
    .rewind_endpoint = parse_ip_and_port(getenv("REWIND_ENDPOINT_2")),
    .glimpse_endpoint = parse_ip_and_port(getenv("GLIMPSE_ENDPOINT_2"))
  };

  return config;
}
