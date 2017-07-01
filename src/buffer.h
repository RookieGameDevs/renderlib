#pragma once

#include <GL/glew.h>

struct Buffer {
	size_t size;
	GLuint vbo;
	GLenum usage_hint;
};

struct Buffer*
buffer_new(size_t size, void *initial_data, GLenum usage_hint);

int
buffer_update(struct Buffer *buf, size_t size, void *data);

void
buffer_free(struct Buffer *buf);