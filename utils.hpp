#pragma once

#include <netinet/in.h>
#include <arpa/inet.h>
#include <string>
#include <string_view>
#include <stdexcept>

namespace utils
{
  sockaddr_in parse_address_from_env(const std::string_view env_var_name)
  {
    const char* env_var = std::getenv(env_var_name.data());
    if (!env_var)
        throw std::runtime_error(env_var_name.data() + std::string(" not set"));

    std::string_view env_str(env_var);
    size_t colon_pos = env_str.find(':');
    if (colon_pos == std::string_view::npos)
        throw std::runtime_error(env_var_name.data() + std::string(" malformed"));

    std::string_view ip = env_str.substr(0, colon_pos);
    std::string_view port = env_str.substr(colon_pos + 1);
    
    if (ip.empty() || port.empty())
        throw std::runtime_error(env_var_name.data() + std::string(" malformed"));

    sockaddr_in addr = {};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(std::stoi(std::string(port)));

    if (inet_pton(AF_INET, ip.data(), &addr.sin_addr) != 1)
      throw std::runtime_error(env_var_name.data() + std::string(" invalid IP"));

    return addr;
  }
}