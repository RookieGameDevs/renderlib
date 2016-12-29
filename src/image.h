#pragma once

#include "errors.h"

struct Image {
	unsigned width, height;
	void *data;
};

struct Image*
image_from_file(const char *filename, err_t *r_err);

void
image_free(struct Image *image);
