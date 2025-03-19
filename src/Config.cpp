#include "Config.hpp"
#include "utils.hpp"

static std::string getenv_or_throw(const char* var);

Config::Config(void) :
  bind_ip(getenv_or_throw("BIND_IP")),
  bind_port_udp(getenv_or_throw("BIND_PORT_UDP")),
  bind_port_tcp(getenv_or_throw("BIND_PORT_TCP")),
  multicast_ip(getenv_or_throw("MULTICAST_IP")),
  multicast_port(getenv_or_throw("MULTICAST_PORT")),
  glimpse_ip(getenv_or_throw("GLIMPSE_IP")),
  glimpse_port(getenv_or_throw("GLIMPSE_PORT")),
  rewind_ip(getenv_or_throw("REWIND_IP")),
  rewind_port(getenv_or_throw("REWIND_PORT")),
  username(getenv_or_throw("USERNAME")),
  password(getenv_or_throw("PASSWORD")) {}

static std::string getenv_or_throw(const char* var)
{
  const char* val = std::getenv(var);
  if (val == nullptr)
    utils::throw_exception("Missing environment variable");
  return val;
}