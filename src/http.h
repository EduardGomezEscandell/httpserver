#include <stdio.h>

#include "reader.h"
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

int headers_append(struct headers_t *headers, char const *key,
                   char const *value);
int headers_get(struct headers_t const *headers, char *buff, size_t buffsize,
                char const *key);

struct request_t {
  char *method;
  char *path;
  char *protocol;
  struct headers_t headers;
  bool error;
  struct reader_t body;
};

struct request_t *parse_request(int fd);
void free_request(struct request_t *req);
size_t request_content_length(struct request_t const *req);
void request_print(struct request_t *req);

struct response_t {
  int fd;
  char *protocol;
  int status;
  struct headers_t headers;
  struct string_t body;
};

struct response_t *new_response(struct request_t *req);
int response_close(struct response_t *res);
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
  int sockfd;
  struct multiplexer_t multiplexer;
};

typedef void (*httpserver_callback)(struct response_t *, struct request_t *);

struct httpserver *new_httpserver();
int httpserver_register(struct httpserver *server, char const *method,
                        char const *path, httpserver_callback handler);
int httpserver_serve(struct httpserver *server, int sockfd);
void httpserver_close(struct httpserver *server);

void callback404(struct response_t *res, struct request_t *req);
void callback405(struct response_t *res, struct request_t *req);