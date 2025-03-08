#include "BufferHandler.hpp"

BufferHandler::BufferHandler(const int fd, const uint16_t read_size, const uint16_t write_size) :
  fd(fd),
  read_buffer(read_size),
  write_buffer(write_size),
  read_offset(0),
  write_offset(0) {}

BufferHandler::~BufferHandler(void) {}

bool BufferHandler::try_read(void)
{
  //TODO non-blocking read
}

bool BufferHandler::try_write(const char *data, const uint16_t len)
{
  //TODO non-blocking write
}

void BufferHandler::reset_buffer(void)
{
  read_offset = 0;
  write_offset = 0;
}