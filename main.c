#include <stdbool.h>
#include <stdio.h>
#include <unistd.h>

#include <netinet/in.h>
#include <sys/sendfile.h>
#include <sys/socket.h>

#include "src/defines.h"
#include "src/http.h"
#include "src/httpcodes.h"
#include "src/net.h"
#include "src/string_t.h"

void handle_home(struct response_t *res, struct request_t *req) {
  res->status = HTTP_STATUS_OK;
  headers_append(&res->headers, "Content-Type", "text/html");
  res->body = new_string_literal("<html><title>Home</title><body><h1>Home page</h1><p>Welcome to the home page!</p></body></html>");
}

void handle_root(struct response_t *res, struct request_t *req) {
  res->status = HTTP_STATUS_PERMANENT_REDIRECT;
  headers_append(&res->headers, "Location", "/home");
}

int main() {
  struct sockaddr_in const addr = {
      .sin_family = AF_INET,
      .sin_port = port(8080),
      .sin_addr = ip_address(127, 0, 0, 1),
  };

  struct httpserver *server = new_httpserver();

  int sockfd = bind_and_listen(&addr, 1024);
  if (sockfd < 0) {
    exiterr(1, "could not bind/listen");
  }

  if (httpserver_register(server, "GET", "/home", handle_home) != 0) {
    exiterr(1, "could not register handler");
  }

  if (httpserver_register(server, "GET", "/", handle_root) != 0) {
    exiterr(1, "could not register handler");
  }

  if (httpserver_serve(server, sockfd) != 0) {
    exiterr(1, "could not serve");
  }

  printf("Closing\n");

  httpserver_close(server);
  close(sockfd);
}
