#include <assert.h>
#include <stdio.h>
#include <unistd.h>

#include <netinet/in.h>
#include <sys/sendfile.h>
#include <sys/socket.h>

#include "defines.h"
#include "net.h"

char *fmt_ip(struct sockaddr_in const *const addr) {
  assert(addr->sin_family == AF_INET);

  uint8_t ip0 = addr->sin_addr.s_addr & 0xff;
  uint8_t ip1 = addr->sin_addr.s_addr >> 8 & 0xff;
  uint8_t ip2 = addr->sin_addr.s_addr >> 16 & 0xff;
  uint8_t ip3 = addr->sin_addr.s_addr >> 24 & 0xff;

  uint16_t p = ntohs(addr->sin_port);

  char *buff =
      malloc(22 * sizeof(*buff)); // IP address=15 + separator=1 + port=5 + \0=1
  sprintf(buff, "%d.%d.%d.%d:%d", ip0, ip1, ip2, ip3, p);
  return buff;
}

in_port_t port(uint16_t p) { return htons(p); }

struct in_addr ip_address(uint8_t a, uint8_t b, uint8_t c, uint8_t d) {
  return (struct in_addr){.s_addr = d << 24 | c << 16 | b << 8 | a};
}

int bind_and_listen(struct sockaddr_in const *const addr, size_t const maxqueue) {
  int sockfd = socket(AF_INET, SOCK_STREAM, 0);
  if (sockfd < 0) {
    exiterr(1, "Could not create socket\n");
  }

  // Allow immediate reuse of the address
  setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &(int){1}, sizeof(int));

  if (bind(sockfd, (struct sockaddr const *)addr, sizeof(*addr)) < 0) {
    exiterrno(1, "could not bind");
  }

  if (listen(sockfd, maxqueue) != 0) {
    exiterrno(1, "could not listen\n");
  }

  char *ip_fmt = fmt_ip(addr);
  printf("Listening to %s\n", ip_fmt);
  free(ip_fmt);

  return sockfd;
}
