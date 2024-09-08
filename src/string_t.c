#include <stdlib.h>
#include <string.h>

#include "string_t.h"

struct string_t null_string() {
  return (struct string_t){
      .data = NULL,
      .len = 0,
      .cap = 0,
  };
}

struct string_t new_string() { return null_string(); }

int string_append(struct string_t *str, char c) {
  if (str->len >= str->cap) {
    str->cap = 16 + str->cap * 2;
    str->data = (char *)realloc(str->data, str->cap * sizeof(*str->data));

    if (str->data == NULL) {
      return 1;
    }
  }

  str->data[str->len] = c;
  ++str->len;
  return 0;
}

void string_pop(struct string_t *str) {
  if (str->len != 0) {
    --str->len;
  }
}

void string_free(struct string_t *str) {
  free(str->data);
  free(str);
}

char *to_cstr(struct string_t *str) {
  string_append(str, '\0');
  char *cstr = str->data;
  *str = new_string();
  return cstr;
}

int string_find_after(struct string_t str, char ch, size_t start) {
  for (; start < str.len; ++start) {
    if (str.data[start] == ch) {
      return start;
    }
  }
  return -1;
}

int string_find(struct string_t str, char ch) {
  return string_find_after(str, ch, 0);
}
