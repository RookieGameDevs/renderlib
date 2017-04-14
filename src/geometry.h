#pragma once

#include "buffer.h"

#include <GL/glew.h>

struct Geometry {
	struct Attribute *attributes;
	unsigned attribute_count;

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
	int type,
	int size,
	int stride,
	void *offset
);

void
geometry_free(struct Geometry *geom);