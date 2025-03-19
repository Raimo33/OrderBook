#pragma once

#include <string_view>
#include <chrono>

#include "Packets.hpp"

class SanityChecker
{
  public:
    SanityChecker(void);
    ~SanityChecker(void) noexcept;

    [[noreturn]] static void throwException(const std::string_view message);

    inline void setSequenceNumber(const uint64_t sequence_number) noexcept;
    inline void checkSequenceNumber(const uint64_t sequence_number) const;
    inline void updateLastReceived(void) noexcept;
    inline void updateLastSent(void) noexcept;
    void validateMessageBlock(const MessageBlock &block) const;

  private:
    const int timer_fd;
    uint64_t sequence_number;
    std::chrono::time_point<std::chrono::steady_clock> last_received;
    std::chrono::time_point<std::chrono::steady_clock> last_sent;
};

#include "SanityChecker.inl"