#include "error.h"
#include "shadow_map.h"
#include <stdlib.h>

struct ShadowMap*
shadow_map_new(unsigned width, unsigned height)
{
	struct ShadowMap *map = malloc(sizeof(struct ShadowMap));
	if (!map) {
		return NULL;
	}
	map->width = width;
	map->height = height;

	// create a texture which will contain depth values
	glGenTextures(1, &map->texture);
	if (!map->texture) {
		err(ERR_OPENGL);
		goto error;
	}
	glBindTexture(GL_TEXTURE_2D, map->texture);
	glTexImage2D(
		GL_TEXTURE_2D,
		0,
		GL_DEPTH_COMPONENT16,
		width,
		height,
		0,
		GL_DEPTH_COMPONENT,
		GL_FLOAT,
		0
	);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	// create a framebuffer object and attach the previously created texture
	// to depth attachment point
	glGenFramebuffers(1, &map->fbo);
	if (!map->fbo || glGetError() != GL_NO_ERROR) {
		err(ERR_OPENGL);
		goto error;
	}
	glBindFramebuffer(GL_FRAMEBUFFER, map->fbo);
	glFramebufferTexture(
		GL_FRAMEBUFFER,
		GL_DEPTH_ATTACHMENT,
		map->texture,
		0
	);
	glReadBuffer(GL_NONE);
	glDrawBuffer(GL_NONE);

	// check framebuffer completeness
	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
		err(ERR_OPENGL);
		goto error;
	}

cleanup:
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glBindTexture(GL_TEXTURE_2D, 0);

	return map;

error:
	shadow_map_free(map);
	map = NULL;
	goto cleanup;
}

void
shadow_map_free(struct ShadowMap *map)
{
	if (map) {
		glDeleteFramebuffers(1, &map->fbo);
		glDeleteTextures(1, &map->texture);
		free(map);
	}
}
