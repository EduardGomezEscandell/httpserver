#include <assert.h>
#include <limits.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include <stdlib.h>

#include <sys/socket.h>

#include "defines.h"
#include "string_t.h"
#include "reader.h"
#include "http.h"

void free_request(struct request_t *req) {
  free(req->type);
  free(req->path);
  free(req->protocol);
  for (size_t i = 0; i < req->headers.len; ++i) {
    free(req->headers.data[i].key);
    free(req->headers.data[i].value);
  }
  free(req);
}

int headers_append(struct headers_t *headers, struct header_t header) {
  if (headers->len == headers->cap) {
    const size_t new_cap = (headers->cap + 1) * 2;
    struct header_t *const new_data =
        realloc(headers->data, new_cap * sizeof(*new_data));
    if (new_data == NULL) {
      return -1;
    }
    headers->data = new_data;
    headers->cap = new_cap;
  }

  headers->data[headers->len] = header;
  ++headers->len;
  return 0;
}


int http_parse_first_line(struct reader_t *r, struct request_t *req) {
  struct string_t line = reader_readline(r, 1024);
  if (r->error) {
    return -1;
  }

  // Parse request type
  int pos0 = string_find(line, ' ');
  req->type = malloc(pos0 + 1);
  memcpy(req->type, line.data, pos0 + 1);
  req->type[pos0] = '\0';

  // Parse path
  ++pos0;
  int pos1 = string_find_after(line, ' ', pos0);
  if (pos1 == -1 || pos1 == pos0) {
    return -2;
  }

  size_t len = pos1 - pos0;
  req->path = malloc(len + 1);
  memcpy(req->path, line.data + pos0, len);
  req->path[len] = '\0';

  // Parse protocol
  ++pos1;
  len = line.len - pos1;
  if (len < 1) {
    return -3;
  }

  req->protocol = malloc(len + 1);
  memcpy(req->protocol, line.data + pos1, len);
  req->protocol[len] = '\0';

  return 0;
}

int http_parse_headers(struct reader_t *r, struct request_t *req) {
  const size_t max_headers = 1024;
  for (size_t i = 0; i < max_headers; ++i) {
    struct string_t line = reader_readline(r, 1024);
    if (r->error) {
      return -2;
    }

    if (line.len == 0) {
      return 0;
    }

    int pos = string_find(line, ':');
    if (pos == -1) {
      return -3;
    }

    // "key: value"
    //     ^pos
    if (pos == 0 || pos > line.len - 2 || line.data[pos + 1] != ' ') {
      return -3;
    }

    struct header_t header = {
        .key = malloc(pos + 1),
        .value = malloc(line.len - pos),
    };

    memcpy(header.key, line.data, pos);
    header.key[pos] = '\0';

    memcpy(header.value, line.data + pos + 2, line.len - pos - 2);
    header.value[line.len - pos - 2] = '\0';

    headers_append(&req->headers, header);
  }

  return -1;
}

size_t request_content_length(struct request_t *req) {
  for (size_t i = 0; i < req->headers.len; ++i) {
    if (strcmp(req->headers.data[i].key, "Content-Length") == 0) {
      return atoll(req->headers.data[i].value);
    }
  }
  return 0;
}

struct request_t *parse_request(int fd) {
  struct request_t *req = malloc(sizeof(*req));
  req->type = NULL;
  req->protocol = NULL;
  req->headers = (struct headers_t){
      .data = NULL,
      .len = 0,
  };
  req->body = (struct reader_t){
      .fd = fd,
      .buf_begin = 0,
      .buf_end = 0,
      .error = false,
  };
  req->error = false;

  // Not really the body yet, will be once we read over the headers
  req->body = (struct reader_t){
      .fd = fd,
      .buffer = {0},
      .buf_begin = 0,
      .buf_end = 0,
      .chars_left = -1,
      .error = false,
  };

  if (http_parse_first_line(&req->body, req) != 0) {
    goto on_error;
  }

  if (http_parse_headers(&req->body, req) != 0) {
    goto on_error;
  }

  req->body.chars_left = request_content_length(req) - reader_bufsize(&req->body);
  return req;

on_error:
  req->error = true;
  req->body.error = true;
  return req;
}

void request_print(struct request_t *req) {
  printf("request {\n");
  printf("  type: %s\n", req->type);
  printf("  path: %s\n", req->path);
  printf("  protocol: %s\n", req->protocol);
  printf("  headers: [\n");
  for (size_t i = 0; i < req->headers.len; ++i) {
    printf("    %s: %s\n", req->headers.data[i].key,
           req->headers.data[i].value);
  }
  printf("  ]\n");
  printf("  body: '''\n");

  for(;;) {
    struct string_t line = reader_read(&req->body, 1024);
    if (req->body.error) {
      break;
    }
    if (line.len == 0) {
      break;
    }
    printf("%s", to_cstr(&line));
  }

  printf("\n  '''\n");
  printf("}\n");
}
