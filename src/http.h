#pragma once

#include "defines.h"
#include "httpcodes.h"
#include "string_t.h"

struct header_t {
  char *key;
  char *value;
};

struct headers_t {
  struct header_t *data;
  size_t len;
  size_t cap;
};

int headers_get(struct headers_t const *headers, char *buff, size_t buffsize,
                char const *key);

#define request_alloc_size 1024

struct request_t {
  char pool[request_alloc_size];
  char *method;
  char *path;
  char *protocol;
  struct headers_t headers;

  char *body;
  size_t content_length;
};

struct request_t *parse_request(int fd);
void free_request(struct request_t *req);
size_t request_content_length(struct request_t const *req);
void request_print(struct request_t *req);

struct response_t {
  int fd;
  char *protocol;
  enum httpcodes status;
  struct headers_t headers;
  struct string_t body;
};

// Create a new response wrapper for the given file descriptor
struct response_t *new_response(int fd);

// Write the response to the fd and free the response
int response_close(struct response_t *res);

// Append a header to the response
int response_headers_append(struct response_t *headers, char *key, char *value);

// Free the response without writing to the socket
// Do not call if response_close was already called
void response_free(struct response_t *res);

struct handler_t {
  char *path;
  char *method;
  void (*handler)(struct response_t *, struct request_t *);
};

struct multiplexer_t {
  struct handler_t *handlers;
  size_t len;
  size_t cap;
};

struct httpserver {
  struct multiplexer_t multiplexer;
};

typedef void (*httpserver_callback)(struct response_t *, struct request_t *);

struct httpserver *new_httpserver();
int httpserver_register(struct httpserver *server, char const *method,
                        char const *path, httpserver_callback handler);
int httpserver_serve(struct httpserver *server, int sockfd);
void httpserver_close(struct httpserver *server);

void callback400(struct response_t *res, struct request_t *req);
void callback404(struct response_t *res, struct request_t *req);
void callback405(struct response_t *res, struct request_t *req);