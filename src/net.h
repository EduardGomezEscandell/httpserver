#include <netinet/in.h>
#include <stdint.h>

in_port_t port(uint16_t p);
struct in_addr ip_address(uint8_t a, uint8_t b, uint8_t c, uint8_t d);
void format_address(char *buff, size_t bufsize, struct sockaddr_in const *addr);
int bind_and_listen(struct sockaddr_in const *addr, size_t maxqueue);
