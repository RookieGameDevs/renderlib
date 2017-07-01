#pragma once

#include "buffer.h"

#include <GL/glew.h>

enum {
	GEOMETRY_TYPE_ARRAY,
	GEOMETRY_TYPE_INDEX,
	GEOMETRY_TYPE_INSTANCED_ARRAY
};

struct Geometry {
	/* read-only */
	struct Attribute *attributes;
	unsigned attribute_count;
	GLuint vao;
	int type;
	union {
		struct {
			unsigned long count;
		} array;
		struct {
			GLenum type;
			unsigned long count;
		} index;
		struct {
			unsigned long count;
			unsigned long primcount;
		} instanced_array;
	};
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
	void *offset,
	int divisor
);

int
geometry_set_index(
	struct Geometry *geom,
	struct Buffer *buf,
	GLenum type,
	unsigned long count
);

int
geometry_set_array(struct Geometry *geom, unsigned long count);

int
geometry_set_instanced_array(struct Geometry *geom, unsigned long count, unsigned long primcount);

int
geometry_bind(struct Geometry *geom);

void
geometry_unbind(struct Geometry *geom);

int
geometry_draw(struct Geometry *geom, GLenum primitive_type);

void
geometry_free(struct Geometry *geom);