#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <netinet/in.h>
#include <sys/sendfile.h>
#include <sys/socket.h>

#include "src/defines.h"
#include "src/http.h"
#include "src/net.h"

int main() {
  struct sockaddr_in const addr = {
      .sin_family = AF_INET,
      .sin_port = port(8080),
      .sin_addr = ip_address(127, 0, 0, 1),
  };

  int sockfd = bind_and_listen(&addr);

  while (true) {
    int fd = accept(sockfd, NULL, NULL);
    if (fd < 0) {
      exiterr(2, "could not accept");
    }

    struct request_t *req = parse_request(fd);

    request_print(req);
    if (req->error) {
      fprintln(fd, "HTTP/1.1 400 Bad Request");
      fprintln(fd, "");
      goto on_close;
    }

    if (strcmp(req->type, "GET") != 0) {
      fprintln(fd, "HTTP/1.1 405 Method Not Allowed");
      fprintln(fd, "Content-Length: 0");
      fprintln(fd, "");
      goto on_close;
    }

    if (strcmp(req->path, "/") != 0) {
      fprintln(fd, "HTTP/1.1 404 Not Found");
      fprintln(fd, "");
      goto on_close;
    }

    fprintln(fd, "HTTP/1.1 200 OK");
    fprintln(fd, "Content-Type: text/plain");
    fprintln(fd, "");
    fprintln(fd, "Hello, World!");
    fprintln(fd, "");
    goto on_close;

  on_close:
    free_request(req);
    close(fd);
  }

  printf("Closing\n");
  close(sockfd);
}
