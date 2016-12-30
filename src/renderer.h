#pragma once

#include "errors.h"

int
renderer_init(err_t *r_err);

int
renderer_present(void);

void
renderer_shutdown(void);
