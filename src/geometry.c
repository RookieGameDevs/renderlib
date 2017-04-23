#include "error.h"
#include "geometry.h"

// standard library
#include <assert.h>
#include <stdlib.h>
#include <string.h>

struct Geometry*
geometry_new(void)
{
	struct Geometry *geom = malloc(sizeof(struct Geometry));
	if (!geom) {
		err(ERR_NO_MEM);
		return NULL;
	}
	memset(geom, 0, sizeof(struct Geometry));

	glGenVertexArrays(1, &geom->vao);
	if (!geom->vao) {
		err(ERR_OPENGL);
		geometry_free(geom);
		return NULL;
	}

	return geom;
}

int
geometry_add_attribute(
	struct Geometry *geom,
	struct Buffer *buf,
	const char *name,
	int type,
	int size,
	int stride,
	void *offset
) {
	assert(geom != NULL);
	assert(buf != NULL);
	assert(name != NULL);
	assert(
		type == GL_FLOAT ||
		type == GL_DOUBLE ||
		type == GL_BYTE ||
		type == GL_UNSIGNED_BYTE ||
		type == GL_SHORT ||
		type == GL_UNSIGNED_SHORT ||
		type == GL_INT ||
		type == GL_UNSIGNED_INT
	);

	// reallocate attributes array for one more attribute
	void *ptr = realloc(
		geom->attributes,
		sizeof(struct Attribute) * (geom->attribute_count + 1)
	);
	if (!ptr) {
		err(ERR_NO_MEM);
		return 0;
	}
	geom->attributes = ptr;
	struct Attribute *attr = &geom->attributes[geom->attribute_count];

	// set attribute data
	attr->name = string_copy(name);
	if (!attr->name) {
		err(ERR_NO_MEM);
		return 0;
	}
	attr->type = type;
	attr->size = size;

	// setup OpenGL attribute pointer
	int ok = 1;
	glBindVertexArray(geom->vao);
	glBindBuffer(GL_ARRAY_BUFFER, buf->vbo);
	if (type == GL_FLOAT) {
		glVertexAttribPointer(
			geom->attribute_count,
			size,
			GL_FLOAT,
			GL_FALSE,
			stride,
			offset
		);
	} else if (type == GL_DOUBLE) {
		glVertexAttribLPointer(
			geom->attribute_count,
			size,
			GL_DOUBLE,
			stride,
			offset
		);
	} else {
		glVertexAttribIPointer(
			geom->attribute_count,
			size,
			type,
			stride,
			offset
		);
	}
	if (glGetError() != GL_NO_ERROR) {
		err(ERR_OPENGL);
		ok = 0;
	} else {
		geom->attribute_count++;
	}
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);

	return ok;
}

void
geometry_free(struct Geometry *geom)
{
	if (geom) {
		for (unsigned i = 0; i < geom->attribute_count; i++) {
			free(geom->attributes[i].name);
		}
		glDeleteVertexArrays(1, &geom->vao);
		free(geom->attributes);
		free(geom);
	}
}