#include <sys/timerfd.h>
#include <stdexcept>
#include <unistd.h>
#include <unordered_map>

#include "SanityChecker.hpp"

SanityChecker::SanityChecker(void) :
  timer_fd(createTimer()),
  sequence_number(0),
  last_received(std::chrono::steady_clock::now()),
  last_sent(std::chrono::steady_clock::now())
{

}

int SanityChecker::createTimer(void) const
{
  bool error = false;

  const int fd = timerfd_create(CLOCK_MONOTONIC, TFD_NONBLOCK);
  error |= (fd == -1);

  struct itimerspec heartbeat_timer{};
  heartbeat_timer.it_interval.tv_sec = 0;
  heartbeat_timer.it_value.tv_nsec = 50'000'000;
  heartbeat_timer.it_interval.tv_sec = 0;
  heartbeat_timer.it_value.tv_nsec = 50'000'000;

  error |= (timerfd_settime(fd, 0, &heartbeat_timer, nullptr) == -1);
  if (error)
    throwException("Failed to initialize timer");

  return fd;
}

SanityChecker::~SanityChecker(void) noexcept
{
  close(timer_fd);
}

[[noreturn]] COLD NEVER_INLINE void SanityChecker::throwException(const std::string_view message)
{
  throw std::runtime_error(message.data());
}

void SanityChecker::validateMessageBlock(const MessageBlock &block)
{
  //TODO constexpr
  static const std::unordered_map<char, uint16_t> message_sizes = {
    {'T', sizeof(MessageBlock::seconds)},
    {'R', sizeof(MessageBlock::series_info_basic)},
    {'M', sizeof(MessageBlock::series_info_basic_combination)},
    {'L', sizeof(MessageBlock::tick_size_data)},
    {'S', sizeof(MessageBlock::system_event_data)},
    {'O', sizeof(MessageBlock::trading_status_data)},
    {'A', sizeof(MessageBlock::new_order)},
    {'E', sizeof(MessageBlock::execution_notice)},
    {'C', sizeof(MessageBlock::execution_notice_with_trade_info)},
    {'D', sizeof(MessageBlock::deleted_order)},
    {'Z', sizeof(MessageBlock::ep)},
    {'G', sizeof(MessageBlock::snapshot_completion)}
  };

  const auto it = message_sizes.find(block.type);
  if (it == message_sizes.end())
    throwException("Unrecognized message type");
}