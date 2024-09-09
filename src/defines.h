#include <stdio.h>
#include <stdlib.h>

#include <errno.h>
#include <string.h>

#define exiterr(retval, msg)                                                   \
  do {                                                                         \
    fprintf(stderr, msg);                                                      \
    exit(retval);                                                              \
  } while (0)

#define exiterrno(retval, msg)                                                 \
  do {                                                                         \
    int const errnum = errno;                                                  \
    char *errmsg = strerror(errnum);                                           \
    fprintf(stderr, "%s: errno %d: %s\n", msg, errnum, errmsg);                \
    exit(retval);                                                              \
  } while (0)

#define fprintln(fd, msg) write(fd, msg "\n", sizeof(msg))
#define dupl_string_literal(literal) strndup(literal, sizeof(literal))