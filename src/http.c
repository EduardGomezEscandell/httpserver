#include <assert.h>
#include <limits.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include <stdlib.h>

#include <sys/socket.h>

#include "defines.h"
#include "http.h"
#include "httpcodes.h"
#include "reader.h"
#include "string_t.h"

int headers_append(struct headers_t *headers, char const *const key,
                   char const *const value) {
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

  headers->data[headers->len] = (struct header_t){
      .key = strndup(key, 1024),
      .value = strndup(value, 4096),
  };

  ++headers->len;
  return 0;
}

void headers_free(struct headers_t *headers) {
  for (size_t i = 0; i < headers->len; ++i) {
    free(headers->data[i].key);
    free(headers->data[i].value);
  }
  free(headers->data);
}

int http_parse_first_line(struct reader_t *r, struct request_t *req) {
  struct string_t line = reader_readline(r, 1024);
  if (r->error) {
    return -1;
  }

  // Parse request type
  int pos0 = string_find(line, ' ');
  req->method = malloc(pos0 + 1);
  memcpy(req->method, line.data, pos0 + 1);
  req->method[pos0] = '\0';

  // Parse path
  ++pos0;
  int pos1 = string_find_after(line, ' ', pos0);
  if (pos1 == -1 || pos1 == pos0) {
    string_free(&line);
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
    string_free(&line);
    return -3;
  }

  req->protocol = malloc(len + 1);
  memcpy(req->protocol, line.data + pos1, len);
  req->protocol[len] = '\0';

  string_free(&line);
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
      string_free(&line);
      return 0;
    }

    int pos = string_find(line, ':');
    if (pos == -1) {
      string_free(&line);
      return -3;
    }

    // "key: value"
    //     ^pos
    if (pos == 0 || pos > line.len - 2 || line.data[pos + 1] != ' ') {
      string_free(&line);
      return -3;
    }

    char *key = line.data;
    key[pos] = '\0';

    char *value = line.data + pos + 2;
    value[line.len - pos - 2] = '\0';

    headers_append(&req->headers, key, value);
    string_free(&line);
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
  req->method = NULL;
  req->protocol = NULL;
  req->path = NULL;
  req->headers = (struct headers_t){
      .data = NULL,
      .len = 0,
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

  req->body.chars_left =
      request_content_length(req) - reader_bufsize(&req->body);
  return req;

on_error:
  req->error = true;
  req->body.error = true;
  return req;
}

void free_request(struct request_t *req) {
  free(req->method);
  free(req->path);
  free(req->protocol);
  headers_free(&req->headers);
  free(req);
}

void request_print(struct request_t *req) {
  printf("request {\n");
  printf("  type: %s\n", req->method);
  printf("  path: %s\n", req->path);
  printf("  protocol: %s\n", req->protocol);
  printf("  headers: [\n");
  for (size_t i = 0; i < req->headers.len; ++i) {
    printf("    %s: %s\n", req->headers.data[i].key,
           req->headers.data[i].value);
  }
  printf("  ]\n");
  printf("  body: '''\n");

  for (;;) {
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

struct response_t *new_response(struct request_t *req) {
  struct response_t *res = malloc(sizeof(*res));

  res->protocol = dupl_string_literal("HTTP/1.1");
  res->status = 200;
  res->headers = (struct headers_t){
      .data = NULL,
      .len = 0,
  };
  res->body = null_string();
  res->fd = req->body.fd;

  return res;
}

int response_close(struct response_t *res) {
  if (res->fd < 0) {
    return -1;
  }

  if (res->status > 999 || res->status < 0) {
    return -2;
  }

  write(res->fd, res->protocol, strlen(res->protocol));
  write(res->fd, " ", 1);
  char status[4];
  snprintf(status, sizeof(status), "%d", res->status);
  write(res->fd, status, strlen(status));
  write(res->fd, " ", 1);

  const char *const reason = httpcode_to_string(res->status);
  if (reason != NULL) {
    write(res->fd, reason, strnlen(reason, 1024));
  }
  write(res->fd, "\n", 1);

  char *content_length = malloc(32);
  snprintf(content_length, 32, "%ld", res->body.len);
  headers_append(&res->headers, "Content-Length", content_length);
  free(content_length);

  for (size_t i = 0; i < res->headers.len; ++i) {
    write(res->fd, res->headers.data[i].key, strlen(res->headers.data[i].key));
    write(res->fd, ": ", 2);
    write(res->fd, res->headers.data[i].value,
          strlen(res->headers.data[i].value));
    write(res->fd, "\n", 1);
  }
  write(res->fd, "\n", 1);
  write(res->fd, res->body.data, res->body.len);
  return 0;
}

void free_response(struct response_t *res) {
  free(res->protocol);
  headers_free(&res->headers);
  free(res->body.data);
  free(res);
}

struct httpserver *new_httpserver() {
  struct httpserver *server = malloc(sizeof(*server));
  server->sockfd = -1;
  server->multiplexer = (struct multiplexer_t){
      .handlers = NULL,
      .len = 0,
      .cap = 0,
  };
  return server;
}

int mux_append(struct multiplexer_t *mux, struct handler_t *handler) {
  if (mux->len == mux->cap) {
    const size_t new_cap = (mux->cap + 1) * 2;
    struct handler_t *const new_data =
        realloc(mux->handlers, new_cap * sizeof(*new_data));
    if (new_data == NULL) {
      return -1;
    }
    mux->handlers = new_data;
    mux->cap = new_cap;
  }

  mux->handlers[mux->len] = *handler;
  ++mux->len;
  return 0;
}

void mux_free(struct multiplexer_t *mux) {
  for (size_t i = 0; i < mux->len; ++i) {
    free(mux->handlers[i].path);
    free(mux->handlers[i].method);
  }
  free(mux->handlers);
}

bool mux_match(char const *const a, char const *const b) {
  return strcmp(a, b) == 0 || strcmp(b, "*") == 0;
}

httpserver_callback mux_get(struct multiplexer_t *mux, char *method,
                            const char *path, int *code) {
  *code = HTTP_STATUS_NOT_FOUND;
  for (size_t i = 0; i < mux->len; ++i) {
    if (mux_match(mux->handlers[i].path, path)) {
      if (mux_match(mux->handlers[i].method, method)) {
        *code = HTTP_STATUS_OK;
        return mux->handlers[i].handler;
      }
      *code = HTTP_STATUS_METHOD_NOT_ALLOWED;
    }
  }

  return NULL;
}

int httpserver_register(struct httpserver *server, char const *method,
                        char const *path, httpserver_callback callback) {
  struct handler_t handler = {
      .path = NULL,
      .method = NULL,
      .handler = NULL,
  };

  handler.path = strndup(path, 2048);
  if (path == NULL) {
    return -1;
  }

  handler.method = strndup(method, 32);
  if (path == NULL) {
    free(handler.path);
    return -1;
  }

  handler.handler = callback;

  if (mux_append(&server->multiplexer, &handler) != 0) {
    free(handler.path);
    free(handler.method);
    return -1;
  }

  return 0;
}

void httpserver_close(struct httpserver *server) {
  if (server->sockfd < 0) {
    return;
  }
  mux_free(&server->multiplexer);
  free(server);
}

int httpserver_serve(struct httpserver *server, int sockfd) {
  server->sockfd = sockfd;
  for (;;) {
    int fd = accept(sockfd, NULL, NULL);
    if (fd < 0) {
      return 1;
    }

    struct request_t *req = parse_request(fd);
    if (req->error) {
      free_request(req);
      continue;
    }

    printf("%s %s\n", req->method, req->path);

    struct response_t *res = new_response(req);
    httpserver_callback callback =
        mux_get(&server->multiplexer, req->method, req->path, &res->status);

    switch (res->status) {
    case HTTP_STATUS_OK:
      callback(res, req);
      break;
    case HTTP_STATUS_NOT_FOUND:
      callback404(res, req);
      break;
    case HTTP_STATUS_METHOD_NOT_ALLOWED:
      callback405(res, req);
      break;
    }

    response_close(res);
    close(fd);

    free_request(req);
    free_response(res);
  }

  return 0;
}

void callback404(struct response_t *res, struct request_t *req) {
  res->status = 404;
  res->body = new_string_literal("404 Not Found\n");
}

void callback405(struct response_t *res, struct request_t *req) {
  res->status = 405;
  res->body = new_string_literal("405 Method Not Allowed\n");
}