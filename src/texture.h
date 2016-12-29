#pragma once

#include "errors.h"
#include <GL/glew.h>

struct Texture {
	GLuint id;
	GLenum type;
};

struct Texture*
texture_from_image(struct Image *image, GLenum type, err_t *r_err);

void
texture_free(struct Texture *tex);
