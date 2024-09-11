#include "httpcodes.h"

char const *const httpcode_to_string(int code) {
  switch (code) {
  case HTTP_STATUS_OK:
    return "OK";
  case HTTP_STATUS_CREATED:
    return "Created";
  case HTTP_STATUS_ACCEPTED:
    return "Accepted";
  case HTTP_STATUS_NON_AUTHORITATIVE_INFORMATION:
    return "Non authoritative information";
  case HTTP_STATUS_NO_CONTENT:
    return "No content";
  case HTTP_STATUS_RESET_CONTENT:
    return "Reset content";
  case HTTP_STATUS_PARTIAL_CONTENT:
    return "Partial content";
  case HTTP_STATUS_MULTI_STATUS:
    return "Multi status";
  case HTTP_STATUS_ALREADY_REPORTED:
    return "Already reported";
  case HTTP_STATUS_IM_USED:
    return "I'm used";
  case HTTP_STATUS_MULTIPLE_CHOICES:
    return "Multiple choices";
  case HTTP_STATUS_MOVED_PERMANENTLY:
    return "Moved permanently";
  case HTTP_STATUS_FOUND:
    return "Found";
  case HTTP_STATUS_SEE_OTHER:
    return "See other";
  case HTTP_STATUS_NOT_MODIFIED:
    return "Not modified";
  case HTTP_STATUS_USE_PROXY:
    return "Use proxy";
  case HTTP_STATUS_TEMPORARY_REDIRECT:
    return "Temporary redirect";
  case HTTP_STATUS_PERMANENT_REDIRECT:
    return "Permanent redirect";
  case HTTP_STATUS_BAD_REQUEST:
    return "Bad request";
  case HTTP_STATUS_UNAUTHORIZED:
    return "Unauthorized";
  case HTTP_STATUS_PAYMENT_REQUIRED:
    return "Payment required";
  case HTTP_STATUS_FORBIDDER:
    return "Forbidder";
  case HTTP_STATUS_NOT_FOUND:
    return "Not found";
  case HTTP_STATUS_METHOD_NOT_ALLOWED:
    return "Method not allowed";
  case HTTP_STATUS_NOT_ACCEPTABLE:
    return "Not acceptable";
  case HTTP_STATUS_PROXY_AUTHENTICATION_REQUIRED:
    return "Proxy authentication required";
  case HTTP_STATUS_REQUEST_TIMEOUT:
    return "Request timeout";
  case HTTP_STATUS_CONFLICT:
    return "Conflict";
  case HTTP_STATUS_GONE:
    return "Gone";
  case HTTP_STATUS_LENGTH_REQUIRED:
    return "Length required";
  case HTTP_STATUS_PRECONDITION_FAILED:
    return "Precondition failed";
  case HTTP_STATUS_PAYLOAD_TOO_LARGE:
    return "Payload too large";
  case HTTP_STATUS_URI_TOO_LONG:
    return "URI too long";
  case HTTP_STATUS_UNSUPPORTED_MEDIA_TYPE:
    return "Unsupported media type";
  case HTTP_STATUS_RANGE_NOT_SATISFIABLE:
    return "Range not satisfiable";
  case HTTP_STATUS_EXPECTATION_FAILED:
    return "Expectation failed";
  case HTTP_STATUS_IM_A_TEAPOT:
    return "I'm a teapot";
  case HTTP_STATUS_MISDIRECTED_REQUEST:
    return "Misdirected request";
  case HTTP_STATUS_UNPROCESSABLE_CONTENT:
    return "Unprocessable content";
  case HTTP_STATUS_LOCKED:
    return "Locked";
  case HTTP_STATUS_FAILED_DEPENDENCY:
    return "Failed dependency";
  case HTTP_STATUS_UPGRADE_REQUIRED:
    return "Upgrade required";
  case HTTP_STATUS_PRECONDITION_REQUIRED:
    return "Precondition required";
  case HTTP_STATUS_TOO_MANY_REQUESTS:
    return "Too many requests";
  case HTTP_STATUS_REQUEST_HEADER_FIELDS_TOO_LARGE:
    return "Request header fields too large";
  case HTTP_STATUS_UNAVAILABLE_FOR_LEGAL_REASONS:
    return "Unavailable for legal reasons";
  case HTTP_STATUS_INTERNAL_SERVER_ERROR:
    return "Internal server error";
  case HTTP_STATUS_NOT_IMPLEMENTED:
    return "Not implemented";
  case HTTP_STATUS_BAD_GATEWAY:
    return "Bad gateway";
  case HTTP_STATUS_SERVICE_UNAVAILABLE:
    return "Service unavailable";
  case HTTP_STATUS_GATEWAY_TIMEOUT:
    return "Gateway timeout";
  case HTTP_STATUS_HTTP_VERSION_NOT_SUPPORTED:
    return "Http version not supported";
  case HTTP_STATUS_VARIANT_ALSO_NEGOTIATES:
    return "Variant also negotiates";
  case HTTP_STATUS_INSUFFICIENT_STORAGE:
    return "Insufficient storage";
  case HTTP_STATUS_LOOP_DETECTED:
    return "Loop detected";
  case HTTP_STATUS_NOT_EXTENDED:
    return "Not extended";
  case HTTP_STATUS_NETWORK_AUTHENTICATION_REQUIRED:
    return "Network authentication required";
  }

  return "Unknown";
}