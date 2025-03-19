#include "SanityChecker.hpp"
#include "macros.hpp"

COLD inline void SanityChecker::setSequenceNumber(const uint64_t sequence_number) noexcept
{
  this->sequence_number = sequence_number;
}

COLD inline void SanityChecker::checkSequenceNumber(const uint64_t sequence_number) const
{
  if (sequence_number != this->sequence_number)
    throwException("Sequence number mismatch");
}

HOT inline void SanityChecker::updateLastReceived(void) noexcept
{
  this->last_received = std::chrono::steady_clock::now();
}

HOT inline void SanityChecker::updateLastSent(void) noexcept
{
  this->last_sent = std::chrono::steady_clock::now();
}