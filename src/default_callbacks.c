#include "http.h"
#include "string_t.h"

#include "default_callbacks.h"

void callback_redirect(struct response_t *res, const char *location) {
  res->status = HTTP_STATUS_MOVED_PERMANENTLY;
  response_headers_append(res, "Location", location);
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