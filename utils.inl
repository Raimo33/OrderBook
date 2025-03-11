#pragma once

#include "utils.hpp"
#include "macros.hpp"

#include <string>
#include <string_view>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <cstdint>
#include <sys/timerfd.h>
#include <immintrin.h>

alignas(64) constexpr uint64_t p10[16] = {
  1ULL,
  10ULL,
  100ULL,
  1000ULL,
  10000ULL,
  100000ULL,
  1000000ULL,
  10000000ULL,
  100000000ULL,
  1000000000ULL,
  10000000000ULL,
  100000000000ULL,
  1000000000000ULL,
  10000000000000ULL,
  100000000000000ULL,
  1000000000000000ULL
};

alignas(64) static const __m512i ascii_zero = _mm512_set1_epi8('0');
alignas(64) static const __m512i tens = _mm512_set1_epi8(10);
alignas(64) static const __m512i powers = _mm512_load_epi64(p10);


static inline size_t try_send(const int sock_fd, const char *buffer, const size_t size, const uint32_t flags);

constexpr __mmask16 calculate_avx512_load_mask(const uint8_t length)
{
  const uint8_t chunks_needed = (length + 3) / 4;
  return static_cast<__mmask16>((1ULL << chunks_needed) - 1);
}

namespace utils
{
  HOT ALWAYS_INLINE inline void assert(const bool condition, const std::string_view message = "Assertion failed")
  {
    if (UNLIKELY(!condition))
      throw_exception(message);
  }

  //TODO stop st first non-digit, cap lengths at the position of the first non-digit, handle 20 digits
  HOT uint64_t parse_u64_simd(std::string_view str) noexcept
  {
    const uint8_t len = str.size();

    const __m512i chars = _mm512_maskz_loadu_epi32(calculate_avx512_load_mask(len), str.data());
    const __m512i digits = _mm512_sub_epi8(chars, ascii_zero);
    const __mmask64 is_digit = _mm512_cmplt_epi8_mask(digits, tens);

    const __mmask64 len_mask = (1ULL << len) - 1;
    const __m512i masked_digits = _mm512_maskz_mov_epi8(len_mask, digits);

    const __m256i digits256 = _mm512_extracti64x4_epi64(masked_digits, 0);
    const __m128i digits128 = _mm256_extracti128_si256(digits256, 0);
    const __m512i digits64 = _mm512_cvtepu8_epi64(digits128);
    const __m512i powers64 = _mm512_load_epi64(p10 + (16 - len));
    const __m512i products = _mm512_mullo_epi64(digits64, powers64);

    return _mm512_reduce_add_epi64(products);
  }

  HOT inline size_t try_tcp_send(const int sock_fd, const char *buffer, const size_t size)
  {
    return try_send(sock_fd, buffer, size, MSG_DONTROUTE | MSG_NOSIGNAL | MSG_DONTWAIT | MSG_ZEROCOPY);
  }

  HOT inline size_t try_udp_send(const int sock_fd, const char *buffer, const size_t size)
  {
    return try_send(sock_fd, buffer, size, MSG_DONTROUTE | MSG_NOSIGNAL | MSG_DONTWAIT);
  }

  HOT inline size_t try_recv(const int sock_fd, char *buffer, const size_t size)
  {
    const ssize_t ret = recv(sock_fd, buffer, size, MSG_DONTWAIT | MSG_NOSIGNAL);

    const bool error = (ret < 0) & !((errno == EAGAIN) | (errno == EWOULDBLOCK));
    if (UNLIKELY(error))
      throw_exception("Failed to receive data");

    return ret;
  }
}

HOT static inline size_t try_send(const int sock_fd, const char *buffer, const size_t size, const uint32_t flags)
{
  const ssize_t ret = send(sock_fd, buffer, size, flags);

  const bool error = (ret < 0) & !((errno == EAGAIN) | (errno == EWOULDBLOCK));
  if (UNLIKELY(error))
    utils::throw_exception("Failed to send data");

  return ret;
}