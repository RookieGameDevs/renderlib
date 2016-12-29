#pragma once

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
image_from_file(const char *filename, const char **r_err);

void
image_free(struct Image *image);
