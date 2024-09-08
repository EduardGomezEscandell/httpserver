#include <stdio.h>

struct header_t {
    char* key;
    char* value;
};

struct headers_t {
    struct header_t *data;
    size_t len;
    size_t cap;
};

struct request_t {
    char* type;
    char* path;
    char* protocol;
    struct headers_t headers;
    char* body;
    bool error;
};

void free_request(struct request_t *req);
struct request_t* parse_request(int fd);
void request_print(struct request_t* req);