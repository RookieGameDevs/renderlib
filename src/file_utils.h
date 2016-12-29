#pragma once

#include <stddef.h>

size_t
file_read(const char *filename, char **r_buf, const char **r_err);
