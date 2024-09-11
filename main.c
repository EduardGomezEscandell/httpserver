#include <stdio.h>
#include <unistd.h>

#include <sys/signal.h>

#include "src/default_callbacks.h"
#include "src/http.h"
#include "src/net.h"
#include "src/settings.h"

void handle_home(struct response_t *res, struct request_t *req) {
  res->status = HTTP_STATUS_OK;
  response_headers_append(res, "Content-Type", "text/html");
  res->body = new_string_literal(
      "<html><title>Home</title><body><h1>Home page</h1><p>Welcome to the home "
      "page!</p></body></html>");
}

void handle_root(struct response_t *res, struct request_t *req) {
  callback_redirect(res, "/home");
}

void handler_parrot(struct response_t *res, struct request_t *req) {
  res->status = HTTP_STATUS_OK;
  char buff[1024];
  if (headers_get(&req->headers, buff, sizeof(buff), "Content-Type") == 0) {
    response_headers_append(res, "Content-Type", buff);
  }

  res->body = new_string(req->body, req->content_length);
}

void handler_sleep(struct response_t *res, struct request_t *req) {
  res->status = HTTP_STATUS_OK;
  res->body = new_string_literal("Sleeping for 1 second\n");
  sleep(1);
}

volatile bool interrupted = false;
void interrupt_handler(int sig) { interrupted = true; }

int main(int argc, char **argv) {
  struct settings settings = parse_cli(argc, argv);

  struct sockaddr_in const addr = {
      .sin_family = AF_INET,
      .sin_port = port(settings.port),
      .sin_addr = ip_address(settings.address),
  };

  struct httpserver *server = new_httpserver();

  int sockfd = bind_and_listen(&addr, 1024);
  if (sockfd < 0) {
    exiterr(1, "could not bind/listen");
  }

  if (httpserver_register(server, "GET", "/home", handle_home) != 0) {
    exiterr(1, "could not register home handler");
  }

  if (httpserver_register(server, "GET", "/", handle_root) != 0) {
    exiterr(1, "could not register root handler");
  }

  if (httpserver_register(server, "POST", "/parrot", handler_parrot) != 0) {
    exiterr(1, "could not register parrot handler");
  }

  if (httpserver_register(server, "POST", "/sleep", handler_sleep) != 0) {
    exiterr(1, "could not register sleep handler");
  }

  char fmt[128];
  format_address(fmt, sizeof(fmt), &addr);
  printf("Listening to %s\n", fmt);
  fflush(stdout);

  signal(SIGINT, interrupt_handler);
  signal(SIGTERM, interrupt_handler);
  signal(SIGQUIT, interrupt_handler);
  sigaddset(&server->interruptmask, SIGINT);
  sigaddset(&server->interruptmask, SIGTERM);
  sigaddset(&server->interruptmask, SIGQUIT);

  if (httpserver_serve(server, sockfd, settings.max_threads, &interrupted) !=
      0) {
    exiterr(1, "could not serve\n");
  }

  httpserver_free(server);
  close(sockfd);

  printf("Exited\n");
  return 0;
}
