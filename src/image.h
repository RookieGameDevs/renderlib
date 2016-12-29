#pragma once

#include "errors.h"
#include <GL/glew.h>
#include <stddef.h>

struct Image {
	GLuint hnd;
	unsigned width, height;
	struct Border {
		uint8_t left, right, top, bottom;
	} border;
};

struct Image*
image_from_file(const char *filename, err_t *r_err);

void
image_free(struct Image *image);
