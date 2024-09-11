#pragma once
#include <stdint.h>

struct settings {
    uint8_t address[4];
    uint16_t port;
    unsigned int max_threads;
};

struct settings parse_cli(int argc, char** argv);
    