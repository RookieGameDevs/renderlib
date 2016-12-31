#pragma once

#include <GL/glew.h>

struct Texture {
	GLuint id;
	GLenum type;
};

struct Texture*
texture_from_image(struct Image *image, GLenum type);

void
texture_free(struct Texture *tex);
