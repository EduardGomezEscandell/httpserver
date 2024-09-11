#include <stdlib.h>

#include "httpcodes.h"

char const *const httpcode_to_string(int code) {
  switch (code) {
  case HTTP_STATUS_OK:
    return "OK";
  case HTTP_STATUS_MOVED_PERMANENTLY:
    return "Moved Permanently";
  case HTTP_STATUS_BAD_REQUEST:
    return "Bad Request";
  case HTTP_STATUS_NOT_FOUND:
    return "Not Found";
  case HTTP_STATUS_METHOD_NOT_ALLOWED:
    return "Method Not Allowed";
  default:
    return NULL;
  }
}