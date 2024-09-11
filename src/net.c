#include <assert.h>

#include "defines.h"
#include "net.h"

void format_address(char *buff, size_t const bufsize,
                    struct sockaddr_in const *addr) {
  assert(addr->sin_family == AF_INET);

  uint8_t ip0 = addr->sin_addr.s_addr & 0xff;
  uint8_t ip1 = addr->sin_addr.s_addr >> 8 & 0xff;
  uint8_t ip2 = addr->sin_addr.s_addr >> 16 & 0xff;
  uint8_t ip3 = addr->sin_addr.s_addr >> 24 & 0xff;

  uint16_t p = ntohs(addr->sin_port);

  snprintf(buff, bufsize, "%d.%d.%d.%d:%d", ip0, ip1, ip2, ip3, p);
}

in_port_t port(uint16_t p) { return htons(p); }

struct in_addr ip_address(uint8_t addr[4]) {
  return (struct in_addr){.s_addr = addr[3] << 24 | addr[2] << 16 |
                                    addr[1] << 8 | addr[0]};
}

int bind_and_listen(struct sockaddr_in const *const addr,
                    size_t const maxqueue) {
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

  return sockfd;
}
