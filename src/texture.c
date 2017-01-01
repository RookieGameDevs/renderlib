#include "error.h"
#include "image.h"
#include "texture.h"
#include <assert.h>
#include <stdlib.h>

struct Texture*
texture_from_image(struct Image *image, GLenum type)
{
	assert(image != NULL);
	assert(type == GL_TEXTURE_2D || type == GL_TEXTURE_RECTANGLE);

	// determine OpenGL pixel formats
	GLenum internal_format, format = 0;
	switch (image->format) {
	case IMAGE_FORMAT_RGB:
		internal_format = GL_RGB8;
		format = GL_RGB;
		break;
	case IMAGE_FORMAT_RGBA:
		internal_format = GL_RGBA8;
		format = GL_RGBA;
		break;
	default:
		err(ERR_TEXTURE_FORMAT);
		return NULL;
	}

	// alloc Texture struct
	struct Texture *tex = malloc(sizeof(struct Texture));
	if (!tex) {
		err(ERR_NO_MEM);
		goto error;
	}
	tex->type = type;

	// create OpenGL texture object
	glGenTextures(1, &tex->id);
	if (!tex->id) {
		err(ERR_OPENGL);
		goto error;
	}

	// bind and initialize the texture
	glBindTexture(type, tex->id);
	glTexParameteri(type, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(type, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(type, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(type, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(type, GL_TEXTURE_BASE_LEVEL, 0);
	glTexParameteri(type, GL_TEXTURE_MAX_LEVEL, 0);
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	glTexImage2D(
		type,
		0,
		format,
		image->width,
		image->height,
		0,
		format,
		GL_UNSIGNED_BYTE,
		image->data
	);
	glPixelStorei(GL_UNPACK_ALIGNMENT, 4);

	// check for any OpenGL errors
	if (glGetError() != GL_NO_ERROR) {
		err(ERR_OPENGL);
		goto error;
	}

cleanup:
	glBindTexture(type, 0);

	return tex;

error:
	texture_free(tex);
	tex = NULL;
	goto cleanup;
}

void
texture_free(struct Texture *tex)
{
	if (tex) {
		glDeleteTextures(1, &tex->id);
		free(tex);
	}
}
