#include <assert.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>

#include "reader.h"
#include "string_t.h"

size_t reader_bufsize(struct reader_t *r) { return r->buf_end - r->buf_begin; }
size_t reader_empty(struct reader_t *r) { return r->buf_end <= r->buf_begin; }
char *reader_bufbegin(struct reader_t *r) { return r->buffer + r->buf_begin; }

size_t reader_advance(struct reader_t *r, size_t n) {
  if (n > reader_bufsize(r)) {
    n = reader_bufsize(r);
  }

  r->buf_begin += n;
  return n;
}

void reader_fillbuffer(struct reader_t *r) {
  assert(r->error == false);

  size_t size_cap = reader_buff_cap;
  if (r->chars_left >= 0 && r->chars_left < reader_buff_cap) {
    size_cap = r->chars_left;
  }

  const size_t size = reader_bufsize(r);
  const ssize_t free_space = size_cap - reader_bufsize(r);
  if (free_space <= 0) {
    return;
  }

  if (size == 0) {
    r->buf_begin = 0;
    r->buf_end = 0;
  } else if (r->buf_begin > 0) {
    memmove(r->buffer, reader_bufbegin(r), size);
    r->buf_end -= r->buf_begin;
    r->buf_begin = 0;
  }

  int code = read(r->fd, r->buffer + r->buf_end, free_space);
  if (code == -1) {
    r->error = true;
    return;
  }

  r->buf_end += code;
  r->chars_left -= code;
}

struct string_t reader_read(struct reader_t *r, size_t maxchars) {
  assert(r->error == false);

  if (maxchars == 0) {
    return null_string();
  }

  struct string_t str = null_string();
  for (;;) {
    size_t sz = reader_bufsize(r);
    if (maxchars < sz) {
      string_append(&str, reader_bufbegin(r), maxchars);
      reader_advance(r, maxchars);
      return str;
    }

    string_append(&str, reader_bufbegin(r), sz);
    reader_advance(r, sz);
    maxchars -= sz;

    if (maxchars == 0) {
      return str;
    }

    reader_fillbuffer(r);
    if(r->error) {
      return null_string();
    }

    if(reader_empty(r)) {
      return str;
    }
  }

  return null_string();
}

struct string_t reader_readline(struct reader_t *r, size_t maxchars) {
  assert(r->error == false);

  bool carriage = false;
  size_t i = 0;
  struct string_t str = null_string();

  for (size_t ibuf = 0; i < maxchars; ++i, ++ibuf) {
    if (ibuf == r->buf_end) {
      string_append(&str, reader_bufbegin(r), ibuf);
      reader_advance(r, ibuf);

      reader_fillbuffer(r);
      if (r->error) {
        goto on_error;
      }

      if (reader_empty(r)) {
        return str;
      }
    }

    const char ch = r->buffer[r->buf_begin + ibuf];
    if (ch == '\n' && !carriage) {
      goto on_error;
    }

    if (ch == '\0') {
      goto on_error;
    }

    if (ch == '\n' && carriage) {
      string_append(&str, reader_bufbegin(r), ibuf);
      reader_advance(r, ibuf + 1); // Skip '\n' character

      // Pop '\r' character
      string_pop(&str);
      return str;
    }

    if (carriage) {
      goto on_error;
    }

    if (ch == '\r') {
      carriage = true;
    }
  }

on_error:
  string_free(&str);
  r->error = true;
  return null_string();
}