#pragma once

int
renderer_init(unsigned width, unsigned height, const char **r_err);

int
renderer_present(void);

void
renderer_shutdown(void);
