#pragma once

#include <netinet/in.h>
#include <arpa/inet.h>
#include <string>
#include <string_view>
#include <stdexcept>

namespace utils
{
  sockaddr_in parse_address_from_env(const char* env_var_name)
  {
    sockaddr_in addr = {};
    addr.sin_family = AF_INET;
  
    const std::string_view env_var = std::getenv(env_var_name);
    if (env_var.empty())
      throw std::runtime_error(std::string(env_var_name) + " not set");
  
    size_t colon_pos = env_var.find(':');
    if (colon_pos == std::string_view::npos)
      throw std::runtime_error(std::string(env_var_name) + " malformed");
  
    const std::string_view ip = env_var.substr(0, colon_pos);
    const std::string_view port = env_var.substr(colon_pos + 1);
  
    if (ip.empty() || port.empty())
      throw std::runtime_error(std::string(env_var_name) + " malformed");
  
    addr.sin_port = htons(std::stoi(port.data()));
    inet_pton(AF_INET, ip.data(), &addr.sin_addr);
  
    return addr;
  }
}