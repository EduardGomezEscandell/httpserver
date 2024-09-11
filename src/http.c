#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <time.h>
#include <unistd.h>

#define __USE_GNU // Required for ppoll and pthread_timedjoin_np
#include <pthread.h>
#include <sys/poll.h>
#undef __USE_GNU

#include <sys/signal.h>
#include <sys/socket.h>

#include "http.h"

// Increase the capacity of the headers_t to make sure one more item fits.
int headers_inc_cap(struct headers_t *headers) {
  if (headers->len < headers->cap) {
    return 0;
  }

  const size_t new_cap = (headers->cap + 1) * 2;
  struct header_t *const new_data =
      realloc(headers->data, new_cap * sizeof(*new_data));
  if (new_data == NULL) {
    return -1;
  }

  headers->data = new_data;
  headers->cap = new_cap;
  return 0;
}

int request_headers_append(struct request_t *req, char *key, char *value) {
  if (headers_inc_cap(&req->headers) != 0) {
    return -1;
  }

  req->headers.data[req->headers.len] = (struct header_t){
      .key = key,
      .value = value,
  };
  ++req->headers.len;

  return 0;
}

int response_headers_append(struct response_t *resp, char *key, char *value) {
  if (headers_inc_cap(&resp->headers) != 0) {
    return -1;
  }

  resp->headers.data[resp->headers.len] = (struct header_t){
      .key = strndup(key, strnlen(key, 4096)),
      .value = strndup(value, strnlen(value, 4096)),
  };
  ++resp->headers.len;

  return 0;
}

int headers_get(struct headers_t const *const headers, char *buff,
                size_t buffsize, char const *const key) {
  assert(buff != NULL);

  for (size_t i = 0; i < headers->len; ++i) {
    if (strcmp(headers->data[i].key, key) == 0) {
      strncpy(buff, headers->data[i].value, buffsize);
      return 0;
    }
  }
  return -1;
}

char *findbefore(char *begin, char const *const end, char target, char *avoid,
                 size_t avoid_len) {
  for (; begin < end; ++begin) {
    if (*begin == target) {
      return begin;
    }
    for (size_t i = 0; i < avoid_len; ++i) {
      if (*begin == avoid[i]) {
        return NULL;
      }
    }
  }

  return NULL;
}

char *http_parse_first_line(struct request_t *req) {
  char *it = req->pool;
  char const *const end = req->pool + request_alloc_size;

  req->method = it;
  it = findbefore(it, end, ' ', "\0\n\r", 3);
  if (it == NULL) {
    return NULL;
  }
  *it = '\0';
  ++it;

  req->path = it;
  it = findbefore(it, end, ' ', "\0\n\r", 3);
  if (it == NULL) {
    return NULL;
  }
  *it = '\0';
  ++it;

  req->protocol = it;
  it = findbefore(it, end, '\r', "\0\n ", 3);
  if (it == NULL) {
    return NULL;
  }
  *it = '\0';

  return it + 2; // Skip \r\n
}

char *http_parse_headers(struct request_t *req, char *it) {
  char *const end = req->pool + request_alloc_size;

  while (it <= end) {
    if (*it == '\r') {

      ++it;
      if (it == end) {
        return NULL;
      }

      if (*it != '\n') {
        return NULL;
      }

      // Empty line
      return it + 1;
    }

    // Parse header key
    char *const key = it;
    it = findbefore(it, end, ':', "\0\n\r", 3);
    if (it == NULL) {
      return NULL;
    }
    *it = '\0';

    // Parse header ' ' separator
    ++it;
    if (it == end || *it != ' ') {
      return NULL;
    }
    ++it;

    // Parse header value
    char *const value = it;
    it = findbefore(it, end, '\r', "\0\n", 2);
    if (it == NULL) {
      return NULL;
    }

    *it = '\0';
    if (request_headers_append(req, key, value) != 0) {
      return NULL;
    }

    ++it;
    if (*it != '\n') {
      return NULL;
    }

    ++it;
  }

  // No empty line found
  return NULL;
}

size_t request_content_length(struct request_t const *const req) {
  char buff[32];
  if (headers_get(&req->headers, buff, sizeof(buff), "Content-Length") != 0) {
    return 0;
  }

  return atoll(buff);
}

int request_init_body(struct request_t *req, int fd, char *it) {
  req->content_length = request_content_length(req);
  if (req->content_length == 0) {
    return 0;
  }

  const size_t pool_slack =
      request_alloc_size - (it - req->pool) - 1; // -1 for null terminator

  // Extra byte for null terminator
  // -> ignored in binary data as it is beyond the content length
  const size_t body_size = req->content_length + 1;

  if (body_size < pool_slack) {
    // Optimize for small bodies: do not allocate
    req->body = it;
    return 0;
  }

  req->body = malloc(body_size);
  if (req->body == NULL) {
    return -1;
  }

  memcpy(req->body, it, pool_slack);
  read(fd, req->body + pool_slack, req->content_length - pool_slack);
  req->body[req->content_length] = '\0';
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
  req->body = NULL;

  int n =
      read(fd, req->pool, request_alloc_size - 1); // -1 to fit null terminator
  if (n <= 0) {
    goto on_error;
  } else if (n < request_alloc_size) {
    req->pool[n] = '\0';
  }

  char *it = http_parse_first_line(req);
  if (it == NULL) {
    goto on_error;
  }

  it = http_parse_headers(req, it);
  if (it == NULL) {
    goto on_error;
  }

  if (request_init_body(req, fd, it) != 0) {
    goto on_error;
  }

  return req;

on_error:
  free_request(req);
  return NULL;
}

void free_request(struct request_t *req) {
  if (req == NULL) {
    return;
  }

  const char *pool_begin = req->pool;
  const char *pool_end = req->pool + request_alloc_size;
  if (pool_begin > req->body || req->body >= pool_end) {
    // Free body when it was allocated
    free(req->body);
  }

  free(req->headers.data);
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
  write(STDOUT_FILENO, req->body, req->content_length);
  printf("\n  '''\n");
  printf("}\n");
}

struct response_t *new_response(int fd) {
  struct response_t *res = malloc(sizeof(*res));
  if (res == NULL) {
    return NULL;
  }

  res->protocol = dupl_string_literal("HTTP/1.1");
  res->status = 200;
  res->headers = (struct headers_t){
      .data = NULL,
      .len = 0,
  };
  res->body = null_string();
  res->fd = fd;

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
  write(res->fd, "\r\n", 2);

  char content_length[32];
  snprintf(content_length, 32, "%ld", res->body.len);
  response_headers_append(res, "Content-Length", content_length);

  for (size_t i = 0; i < res->headers.len; ++i) {
    write(res->fd, res->headers.data[i].key, strlen(res->headers.data[i].key));
    write(res->fd, ": ", 2);
    write(res->fd, res->headers.data[i].value,
          strlen(res->headers.data[i].value));
    write(res->fd, "\r\n", 2);
  }
  write(res->fd, "\r\n", 2);
  write(res->fd, res->body.data, res->body.len);

  response_free(res);
  return 0;
}

void response_free(struct response_t *res) {
  free(res->protocol);

  // Unlike request_t, headers are not allocated in a pool
  for (size_t i = 0; i < res->headers.len; ++i) {
    free(res->headers.data[i].key);
    free(res->headers.data[i].value);
  }
  free(res->headers.data);
  free(res->body.data);
  free(res);
}

struct httpserver *new_httpserver() {
  struct httpserver *server = malloc(sizeof(*server));
  server->multiplexer = (struct multiplexer_t){
      .handlers = NULL,
      .len = 0,
      .cap = 0,
  };

  sigemptyset(&server->interruptmask);
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

httpserver_callback mux_get(struct multiplexer_t const *mux, char *method,
                            const char *path) {

  // Default to Not Found
  httpserver_callback callback = callback404;

  for (size_t i = 0; i < mux->len; ++i) {
    if (mux_match(mux->handlers[i].path, path)) {
      if (mux_match(mux->handlers[i].method, method)) {
        return mux->handlers[i].handler;
      }

      // Path matched but method did not
      callback = callback405;
    }
  }

  return callback;
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

void httpserver_free(struct httpserver *server) {
  mux_free(&server->multiplexer);
  free(server);
}

int httpserver_wait_accept(sigset_t const sigmask, int const sockfd) {
  struct pollfd fds = {
      .fd = sockfd,
      .events = POLLIN,
  };

  struct timespec const timeout = {
      .tv_sec = 1,
      .tv_nsec = 0,
  };

  int ret = ppoll(&fds, 1, &timeout, &sigmask);
  if (ret <= 0) {
    return ret;
  }

  return accept(sockfd, NULL, NULL);
}

struct connection_details {
  struct httpserver *server;
  int fd;
  int thread_id;
};

void handle_connection_imp(struct connection_details const *const cd) {
  struct request_t *req = parse_request(cd->fd);
  struct response_t *res = new_response(cd->fd);

  if (res == NULL) {
    write(cd->fd, "HTTP/1.1 500 Internal Server Error\n\n", 36);
    free_request(req);
    return;
  }

  httpserver_callback callback;
  if (req == NULL) {
    printf("bad request (thread %d)\n", cd->thread_id);
    callback = callback400; // Bad Request
  } else {
    printf("%s %s (thread %d)\n", req->method, req->path, cd->thread_id);
    callback = mux_get(&cd->server->multiplexer, req->method, req->path);
  }

  callback(res, req);

  free_request(req);
  response_close(res);
}

void *handle_connection(void *ptr) {
  struct connection_details *const cd = (struct connection_details *)ptr;
  handle_connection_imp(cd);
  close(cd->fd);
  free(ptr);
  return NULL;
}

struct thread_spinner {
  pthread_t *threads;
  size_t size;
  size_t cursor;
};

struct thread_spinner new_thread_spinner(size_t const size) {
  struct thread_spinner spinner = {
      .threads = calloc(size, sizeof(pthread_t)),
      .size = size,
      .cursor = 0,
  };

  if (spinner.threads == NULL) {
    spinner.size = 0;
  }
  return spinner;
}

void thread_spinner_close(struct thread_spinner *t, volatile bool *interrupt) {
  for (size_t i = 0; i < t->size && !interrupt; ++i) {
    if (t->threads[i] == 0) {
      continue;
    }

    pthread_join(t->threads[i], NULL);
    t->threads[i] = 0;
  }

  free(t->threads);
  t->threads = NULL;
  t->size = 0;
  t->cursor = 0;
}

int thread_spinner_get(struct thread_spinner *t, volatile bool *interrupt) {
  while (!*interrupt) {
    if (t->threads[t->cursor] == 0) { // Free slot
      size_t const i = t->cursor;
      t->cursor = (t->cursor + 1) % t->size;
      return i;
    }

    struct timespec abstime;
    clock_gettime(CLOCK_REALTIME, &abstime);
    const size_t millisecond = 1000 * 1000;
    abstime.tv_nsec += 10 * millisecond;

    const size_t i = t->cursor;
    t->cursor = (t->cursor + 1) % t->size;

    int x = pthread_timedjoin_np(t->threads[i], NULL, &abstime);
    switch (x) {
    case 0:
      // Slot opened up
      t->threads[i] = 0;
      return i;
    case EBUSY:
      /* fallthrough */
    case ETIMEDOUT:
      continue;
    }

    return -1;
  }

  return -1;
}

int httpserver_serve(struct httpserver *const server, const int sockfd,
                     const size_t max_threads, volatile bool *interrupt) {
  bool dummy = false;
  if (interrupt == NULL) {
    interrupt = &dummy;
  }

  struct thread_spinner tspinner = new_thread_spinner(max_threads);
  if (tspinner.threads == NULL) {
    return -1;
  }

  while (!*interrupt) {
    int fd = httpserver_wait_accept(server->interruptmask, sockfd);
    if (fd < 0) {
      thread_spinner_close(&tspinner, interrupt);
      return -1;
    } else if (fd == 0) {
      continue;
    }

    int idx = thread_spinner_get(&tspinner, interrupt);
    if (idx < 0) {
      close(fd);
      continue;
    }

    pthread_t *thread = &tspinner.threads[idx];

    struct connection_details *deets = malloc(sizeof(*deets));
    *deets = (struct connection_details){
        .server = server,
        .fd = fd,
        .thread_id = idx,
    };

    if (pthread_create(thread, NULL, handle_connection, (void *)deets) != 0) {
      free(deets);
      close(fd);
      *thread = 0; // Mark slot as free
      continue;
    }
  }

  *interrupt = false;
  thread_spinner_close(&tspinner, interrupt);
  return 0;
}

void callback400(struct response_t *res, struct request_t *req) {
  res->status = HTTP_STATUS_BAD_REQUEST;
  if (req == NULL) {
    // could be NULL if the request could not be parsed
    return;
  }

  if (strncmp(req->method, "HEAD", sizeof("HEAD")) == 0) {
    // HEAD is not allowed to have a body
    return;
  }

  res->body = new_string_literal("400 Bad Request\n");
}

void callback404(struct response_t *res, struct request_t *req) {
  res->status = HTTP_STATUS_NOT_FOUND;
  if (strncmp(req->method, "HEAD", sizeof("HEAD")) == 0) {
    // HEAD is not allowed to have a body
    return;
  }
  res->body = new_string_literal("404 Not Found\n");
}

void callback405(struct response_t *res, struct request_t *req) {
  res->status = HTTP_STATUS_METHOD_NOT_ALLOWED;
  if (strncmp(req->method, "HEAD", sizeof("HEAD")) == 0) {
    // HEAD is not allowed to have a body
    return;
  }
  res->body = new_string_literal("405 Method Not Allowed\n");
}