#include <sys/timerfd.h>
#include <sys/signalfd.h>
#include <signal.h>
#include <stdexcept>
#include <unistd.h>
#include <unordered_map>

#include "SanityChecker.hpp"

using namespace std::chrono_literals;

SanityChecker::SanityChecker(void) :
  timer_fd(createTimerFd(50ms)),
  signal_fd(createSignalFd(SIGTERM | SIGINT | SIGPIPE)),
  sequence_number(0),
  last_received(std::chrono::steady_clock::now()),
  last_sent(std::chrono::steady_clock::now())
{
  //TODO setup timer handler
  //TODO setup signal handler
}

int SanityChecker::createTimerFd(const std::chrono::milliseconds interval) const
{
  bool error = false;

  const int fd = timerfd_create(CLOCK_MONOTONIC, TFD_NONBLOCK);
  error |= (fd == -1);

  time_t sec = interval.count() / 1000;
  long nsec = (interval.count() % 1000) * 1000000;

  itimerspec heartbeat_timer = {
    .it_interval = { sec, nsec },
    .it_value = { sec, nsec }
  };

  error |= (timerfd_settime(fd, 0, &heartbeat_timer, nullptr) == -1);

  if (error)
    throwException("Failed to initialize timer");

  return fd;
}

int SanityChecker::createSignalFd(const int sigmask) const
{
  bool error = false;

  sigset_t mask;
  error |= (sigemptyset(&mask) == -1);
  error |= (sigaddset(&mask, sigmask) == -1);
  error |= (sigprocmask(SIG_BLOCK, &mask, nullptr) == -1);

  const int fd = signalfd(-1, &mask, SFD_NONBLOCK);
  error |= (fd == -1);

  if (error)
    throwException("Failed to initialize signal fd");

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