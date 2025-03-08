#pragma once

#include <cstdint>
#include <vector>

class BufferHandler
{
  public:
    BufferHandler(int fd, const uint16_t read_size, const uint16_t write_size);
    ~BufferHandler(void);

    const int fd;

    bool try_read(void);
    bool try_write(const char *data, const uint16_t len);
    void reset_buffer(void);

  private:
    std::vector<char> read_buffer;
    std::vector<char> write_buffer;
    uint16_t read_offset;
    uint16_t write_offset;
};