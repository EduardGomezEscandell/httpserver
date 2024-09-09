#pragma once

enum httpcodes {
  HTTP_STATUS_OK = 200,

  HTTP_STATUS_TEMPORARY_REDIRECT = 307,
  HTTP_STATUS_PERMANENT_REDIRECT = 308,

  HTTP_STATUS_NOT_FOUND = 404,
  HTTP_STATUS_METHOD_NOT_ALLOWED = 405,
};

char const *const httpcode_to_string(int code);