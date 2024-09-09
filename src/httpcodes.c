#include "httpcodes.h"
#include <stdlib.h>

char const *const httpcode_to_string(int code) {
  switch (code) {
  case HTTP_STATUS_OK:
    return "OK";
  case HTTP_STATUS_NOT_FOUND:
    return "Not Found";
  case HTTP_STATUS_METHOD_NOT_ALLOWED:
    return "Method Not Allowed";
  default:
    return NULL;
  }
}