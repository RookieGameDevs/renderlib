#include "buffer.h"
#include "error.h"

// standard library
#include <assert.h>
#include <stdlib.h>
#include <string.h>

struct Buffer*
buffer_new(size_t size, void *initial_data, GLenum usage_hint)
{
	struct Buffer *buf = malloc(sizeof(struct Buffer));
	if (!buf) {
		err(ERR_NO_MEM);
		return NULL;
	}
	memset(buf, 0, sizeof(struct Buffer));
	buf->usage_hint = usage_hint;

	// create a buffer
	glGenBuffers(1, &buf->vbo);
	if (!buf->vbo) {
		err(ERR_OPENGL);
		goto buffer_new_error;
	}

	// initialize buffer storage
	glBindBuffer(GL_ARRAY_BUFFER, buf->vbo);
	glBufferData(GL_ARRAY_BUFFER, size, initial_data, usage_hint);
	if (glGetError() != GL_NO_ERROR) {
		err(ERR_OPENGL);
		goto buffer_new_error;
	}

cleanup:
	glBindBuffer(GL_ARRAY_BUFFER, 0);

	return buf;

buffer_new_error:
	buffer_free(buf);
	buf = NULL;
	goto cleanup;
}

int
buffer_update(struct Buffer *buf, size_t size, void *data)
{
	assert(buf != NULL);
	glBindBuffer(GL_ARRAY_BUFFER, buf->vbo);
	if (buf->size > size) {
		glBufferSubData(GL_ARRAY_BUFFER, 0, size, data);
	} else {
		glBufferData(GL_ARRAY_BUFFER, size, data, buf->usage_hint);
	}
	if (glGetError() != GL_NO_ERROR) {
		err(ERR_OPENGL);
		return 0;
	}
	buf->size = size;
	return 1;
}

void
buffer_free(struct Buffer *buf)
{
	if (buf) {
		glDeleteBuffers(1, &buf->vbo);
		free(buf);
	}
}