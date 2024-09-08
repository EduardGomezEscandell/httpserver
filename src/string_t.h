#pragma once

#include <stdlib.h>

struct string_t {
  char *data;
  size_t len;
  size_t cap;
};

struct string_t null_string();
struct string_t new_string(const char *cstr, size_t len);

int string_reserve(struct string_t *str, size_t newcap);
int string_append(struct string_t *str, const char *cstr, size_t len);

int string_push(struct string_t *str, char c);
void string_pop(struct string_t *str);

void string_free(struct string_t *str);
char *to_cstr(struct string_t *str);

int string_find(struct string_t str, char ch);
int string_find_after(struct string_t str, char ch, size_t start);
