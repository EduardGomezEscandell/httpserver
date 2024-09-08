#pragma once

#include <stdlib.h>

#define reader_buff_cap 256

struct reader_t {
  int fd;
  char buffer[reader_buff_cap];
  size_t buf_begin;
  size_t buf_end;
  ssize_t chars_left;
  bool error;
};

size_t reader_bufsize(struct reader_t *r);
struct string_t reader_read(struct reader_t *r, size_t maxchars);
struct string_t reader_readline(struct reader_t *r, size_t maxchars);