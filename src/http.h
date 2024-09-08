#include <stdio.h>

#include "reader.h"


struct header_t {
  char *key;
  char *value;
};

struct headers_t {
  struct header_t *data;
  size_t len;
  size_t cap;
};

struct request_t {
  char *type;
  char *path;
  char *protocol;
  struct headers_t headers;
  bool error;
  struct reader_t body;
};

struct request_t *parse_request(int fd);

void free_request(struct request_t *req);
void request_print(struct request_t *req);
