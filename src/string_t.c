#include <stdarg.h>
#include <stdio.h>
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

struct string_t new_string(const char *cstr, size_t len) {
  struct string_t str = null_string();
  if (len == 0) {
    return str;
  }

  if(string_reserve(&str, len) != 0) {
    return null_string();
  }

  memcpy(str.data, cstr, len);
  str.len = len;
  return str;
}

int string_reserve(struct string_t *str, size_t newcap) {
  if (newcap <= str->cap) {
    return 0;
  }

  newcap = (newcap > 2*str->cap) ? newcap : 2*str->cap;

  if (str->data == NULL) {
    str->data = malloc(newcap * sizeof(*str->data));
    if (str->data == NULL) {
      return 1;
    }
    str->cap = newcap;
    return 0;
  }
  
  str->data = realloc(str->data, newcap * sizeof(*str->data));
  if (str->data == NULL) {
    return 1;
  }

  str->cap = newcap;
  return 0;
}

int string_append(struct string_t *str, const char *cstr, size_t len) {
  if(len == 0) {
    return 0;
  }

  if(string_reserve(str, str->len + len) != 0) {
    return 1;
  }

  memcpy(str->data + str->len, cstr, len);
  str->len += len;
  return 0;
}

int string_push(struct string_t *str, char c) {
  if(string_reserve(str, str->len + 1) != 0) {
    return 1;
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
}

char *to_cstr(struct string_t *str) {
  string_push(str, '\0');
  char *cstr = str->data;
  *str = null_string();
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
