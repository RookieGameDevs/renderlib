#pragma once

#include <GL/glew.h>

struct ShadowMap {
	GLuint fbo;
	GLuint texture;
	unsigned width;
	unsigned height;
};

struct ShadowMap*
shadow_map_new(unsigned width, unsigned height);

void
shadow_map_free(struct ShadowMap *map);
