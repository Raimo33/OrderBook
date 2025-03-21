#include "Config.hpp"
#include "error.h"

static std::string safe_getenv(const char* var);

Config::Config(void) :
  bind_ip(safe_getenv("BIND_IP")),
  bind_port_udp(safe_getenv("BIND_PORT_UDP")),
  bind_port_tcp(safe_getenv("BIND_PORT_TCP")),
  multicast_ip(safe_getenv("MULTICAST_IP")),
  multicast_port(safe_getenv("MULTICAST_PORT")),
  glimpse_ip(safe_getenv("GLIMPSE_IP")),
  glimpse_port(safe_getenv("GLIMPSE_PORT")),
  rewind_ip(safe_getenv("REWIND_IP")),
  rewind_port(safe_getenv("REWIND_PORT")),
  username(safe_getenv("USERNAME")),
  password(safe_getenv("PASSWORD"))
{
  CHECK_ERROR;
}

static std::string safe_getenv(const char* var)
{
  const char* val = std::getenv(var);
  error |= (val == nullptr);
  return val;
}