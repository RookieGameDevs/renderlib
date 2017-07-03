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
	GLenum type,
	int size,
	int stride,
	void *offset,
	int divisor
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
	glEnableVertexAttribArray(geom->attribute_count);
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

	// set the divisor
	glVertexAttribDivisor(geom->attribute_count, divisor);

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

static void
geometry_reset_elements_descriptor(struct Geometry *geom)
{
	// unbind any element array associated with the geometry
	glBindVertexArray(geom->vao);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
	glBindVertexArray(0);

	// zero the union part of the geometry struct
	size_t dscr_offset = offsetof(struct Geometry, type) + sizeof(int);
	size_t dscr_size = sizeof(struct Geometry) - dscr_offset;
	memset(((void*)geom) + dscr_offset, 0, dscr_size);
}

int
geometry_set_index(
	struct Geometry *geom,
	struct Buffer *buf,
	GLenum type,
	unsigned long count
) {
	assert(geom != NULL);
	assert(buf != NULL);
	assert(
		type == GL_UNSIGNED_BYTE ||
		type == GL_UNSIGNED_SHORT ||
		type == GL_UNSIGNED_INT
	);

	geometry_reset_elements_descriptor(geom);
	glBindVertexArray(geom->vao);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, buf->vbo);
	geom->type = GEOMETRY_TYPE_INDEX;
	geom->index.type = type;
	geom->index.count = count;
	glBindVertexArray(0);

	return glGetError() == GL_NO_ERROR;
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

int
geometry_set_array(struct Geometry *geom, unsigned long count)
{
	assert(geom);
	geometry_reset_elements_descriptor(geom);
	geom->type = GEOMETRY_TYPE_ARRAY;
	geom->array.count = count;
	return 1;
}

int
geometry_set_instanced_array(struct Geometry *geom, unsigned long count, unsigned long primcount)
{
	assert(geom);
	geometry_reset_elements_descriptor(geom);
	geom->type = GEOMETRY_TYPE_INSTANCED_ARRAY;
	geom->instanced_array.count = count;
	geom->instanced_array.primcount = primcount;
	return 1;
}

int
geometry_bind(struct Geometry *geom)
{
	assert(geom);
	glBindVertexArray(geom->vao);
	return glGetError() == GL_NO_ERROR;
}

void
geometry_unbind(struct Geometry *geom)
{
	assert(geom);
	glBindVertexArray(0);
}

int
geometry_draw(struct Geometry *geom, GLenum primitive_type)
{
	assert(geom);
	assert(
		primitive_type == GL_LINES ||
		primitive_type == GL_LINES_ADJACENCY ||
		primitive_type == GL_LINE_LOOP ||
		primitive_type == GL_LINE_STRIP ||
		primitive_type == GL_LINE_STRIP_ADJACENCY ||
		primitive_type == GL_PATCHES ||
		primitive_type == GL_POINTS ||
		primitive_type == GL_TRIANGLES ||
		primitive_type == GL_TRIANGLES_ADJACENCY ||
		primitive_type == GL_TRIANGLE_FAN ||
		primitive_type == GL_TRIANGLE_STRIP ||
		primitive_type == GL_TRIANGLE_STRIP_ADJACENCY
	);

	switch (geom->type) {
	case GEOMETRY_TYPE_ARRAY:
		glDrawArrays(primitive_type, 0, geom->array.count);
		break;
	case GEOMETRY_TYPE_INDEX:
		glDrawElements(
			primitive_type,
			geom->index.count,
			geom->index.type,
			(GLvoid*)0L
		);
		break;
	case GEOMETRY_TYPE_INSTANCED_ARRAY:
		glDrawArraysInstanced(
			primitive_type,
			0,
			geom->instanced_array.count,
			geom->instanced_array.primcount
		);
		break;
	}

	return glGetError() == GL_NO_ERROR;
}