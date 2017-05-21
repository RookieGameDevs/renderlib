#pragma once

#include "buffer.h"

#include <GL/glew.h>

struct Geometry {
	struct Attribute *attributes;
	unsigned attribute_count;

	GLenum elements_type;
	unsigned long elements_count;

	GLuint vao;
};

struct Attribute {
	char *name;
	int type;
	int size;
};

struct Geometry*
geometry_new(void);

int
geometry_add_attribute(
	struct Geometry *geom,
	struct Buffer *buf,
	const char *name,
	GLenum type,
	int size,
	int stride,
	void *offset
);

int
geometry_set_index(
	struct Geometry *geom,
	struct Buffer *buf,
	GLenum type,
	unsigned long count
);

void
geometry_free(struct Geometry *geom);