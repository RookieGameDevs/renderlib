#pragma once

#include "errors.h"
#include <stddef.h>

size_t
file_read(const char *filename, char **r_buf, err_t *r_err);
