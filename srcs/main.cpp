/*================================================================================

File: main.cpp                                                                  
Creator: Claudio Raimondi                                                       
Email: claudio.raimondi@pm.me                                                   

created at: 2025-03-08 18:21:38                                                 
last edited: 2025-04-24 12:40:43                                                

================================================================================*/

#include <csignal>

#include "Client.hpp"
#include "Config.hpp"
#include "error.hpp"

volatile bool error = false;

//TODO disaster recovery
//TODO glimpse
//TODO rewind

void init_signal_handler(void);

int main(int argc, char **argv)
{
  if (argc != 3)
    return 1;

  init_signal_handler();

  const Config config = {
    .bind_ip = "10.248.193.12",
    .multicast_ip = "239.194.169.2",
    .multicast_port = "21002",
    .rewind_ip = "10.18.146.3",
    .rewind_port = "24003",
    .glimpse_ip = "10.18.146.3",
    .glimpse_port = "21815",
    .book_ids = {
      53018725, 77267045, 128122981, 59965541, 66715749, 196709, 1114213, 51970149, 393317, 52363365, 66584677, 14090341, 60031077, 84279397, 262245, 52953189, 458853, 128057445, 65732709, 66650213, 36765797, 1048677, 77070437
    },
    .username = argv[1],
    .password = argv[2]
  };

  Client client(config);
  client.run();
}

COLD void init_signal_handler(void)
{
  struct sigaction sa{};

  sa.sa_handler = [](int) { panic(); };

  error |= sigaction(SIGINT, &sa, nullptr) == -1;
  error |= sigaction(SIGTERM, &sa, nullptr) == -1;
  error |= sigaction(SIGQUIT, &sa, nullptr) == -1;
  error |= sigaction(SIGPIPE, &sa, nullptr) == -1;

  CHECK_ERROR;
}