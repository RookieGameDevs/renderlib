#pragma once

#include <stddef.h>

char*
string_fmt(const char *fmt, ...);

char*
string_copy(const char *str);

char*
string_replace(const char *src, const char *pat, const char *repl);

char*
string_join(const char **strings, const char *sep);

char**
string_split(const char *s, char ch, size_t *r_count);

void
string_freev(char **strv, size_t count);
