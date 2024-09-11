#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sys/sysinfo.h>

#include "settings.h"

// Finite state machine to parse command line arguments
enum stage {
  NONE,
  ERROR,
  ADDR,
  PORT,
  MAX_THREADS,
};

enum stage next_word_NONE(char const *word);
enum stage next_word_ADDR(struct settings *settings, char const *word);
enum stage next_word_PORT(struct settings *settings, char const *word);
enum stage next_word_MAX_THREADS(struct settings *setting, char const *word);

void print_help();

struct settings parse_cli(int argc, char **argv) {
  struct settings settings = {
      .address = {127, 0, 0, 1},
      .port = 8080,
      .max_threads = get_nprocs(),
  };

  enum stage status = NONE;

  for (int i = 1; i < argc; ++i) {
    switch (status) {
    case NONE:
      status = next_word_NONE(argv[i]);
      break;
    case ADDR:
      status = next_word_ADDR(&settings, argv[i]);
      break;
    case PORT:
      status = next_word_PORT(&settings, argv[i]);
      break;
    case MAX_THREADS:
      status = next_word_MAX_THREADS(&settings, argv[i]);
      break;
    case ERROR:
      break;
    }
  }

  switch (status) {
  case NONE:
    return settings;
  case ADDR:
    fprintf(stderr, "Missing argument ADDR\n");
    break;
  case PORT:
    fprintf(stderr, "Missing argument PORT\n");
    break;
  case MAX_THREADS:
    fprintf(stderr, "Missing argument NUM\n");
    break;
  case ERROR:
    break;
  }

  fprintf(stderr, "Use --help to see usage\n");
  exit(2);
}

enum stage next_word_NONE(char const *const word) {
  if (strcmp(word, "--help") == 0 || strcmp(word, "-h") == 0) {
    print_help();
    exit(0);
  }

  if (strcmp(word, "--address") == 0 || strcmp(word, "-a") == 0) {
    return ADDR;
  }

  if (strcmp(word, "--port") == 0 || strcmp(word, "-p") == 0) {
    return PORT;
  }

  if (strcmp(word, "--threads") == 0 || strcmp(word, "-t") == 0) {
    return MAX_THREADS;
  }

  fprintf(stderr, "Unexpected argument: %s\n", word);
  return ERROR;
}

enum stage next_word_ADDR(struct settings *settings, char const *const word) {
  char *end;
  settings->address[0] = strtol(word, &end, 10);
  if (*end != '.') {
    fprintf(stderr, "Could not parse address: %s\n", word);
    return ERROR;
  }

  settings->address[1] = strtol(end + 1, &end, 10);
  if (*end != '.') {
    fprintf(stderr, "Could not parse address: %s\n", word);
    return ERROR;
  }

  settings->address[2] = strtol(end + 1, &end, 10);
  if (*end != '.') {
    fprintf(stderr, "Could not parse address: %s\n", word);
    return ERROR;
  }

  settings->address[3] = strtol(end + 1, &end, 10);
  if (*end != '\0') {
    fprintf(stderr, "Could not parse address: %s\n", word);
    return ERROR;
  }

  return NONE;
}

enum stage next_word_PORT(struct settings *settings, char const *const word) {
  char *end;
  settings->port = strtol(word, &end, 10);
  if (*end != '\0') {
    fprintf(stderr, "Could not parse port: %s\n", word);
    return ERROR;
  }

  return NONE;
}

enum stage next_word_MAX_THREADS(struct settings *settings,
                                 char const *const word) {
  char *end;
  settings->max_threads = strtol(word, &end, 10);
  if (*end != '\0') {
    fprintf(stderr, "Could not parse number of threads: %s\n", word);
    return ERROR;
  }

  return NONE;
}

void print_help() {
  printf("Usage: httpserver [OPTION]...\n");
  printf("Start a simple HTTP server\n\n");
  printf("Options:\n");
  printf("  -h, --help\t\t\tDisplay this help and exit\n");
  printf("  -a, --address ADDR\t\tBind to ADDR (default: 127.0.0.1)\n");
  printf("  -p, --port PORT\t\tListen on PORT (default: 8080)\n");
  printf("  -t, --threads NUM\t\tUse NUM threads (default: %d)\n",
         get_nprocs());
}
