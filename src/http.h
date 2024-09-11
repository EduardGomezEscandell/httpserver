#pragma once

#include <stdbool.h>

#include <sys/signal.h>

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
  enum http_status status;
  struct headers_t headers;
  struct string_t body;
};

// Create a new response wrapper for the given file descriptor
struct response_t *new_response(int fd);

// Write the response to the fd and free the response
int response_close(struct response_t *res);

// Append a header to the response
int response_headers_append(struct response_t *headers, char const *key,
                            char const *value);

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
  // Multiplexer for the server, mapping paths to handlers
  struct multiplexer_t multiplexer;

  // Mask with signals that are handler externally
  sigset_t interruptmask;
};

typedef void (*httpserver_callback)(struct response_t *, struct request_t *);

// Create a new http server
struct httpserver *new_httpserver();

// Register a handler for the given method and path
// Either may be "*" to match any method or path
int httpserver_register(struct httpserver *server, char const *method,
                        char const *path, httpserver_callback handler);

// Serve the http server on the given socket file descriptor
// If interrupt is not NULL, it'll be used to stop the server when set to true
int httpserver_serve(struct httpserver *server, int sockfd, size_t max_threads,
                     volatile bool *interrupt);

// Close the http server and free its resources
// Does not close the socket file descriptor
void httpserver_free(struct httpserver *server);
