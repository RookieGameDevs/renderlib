#include "error.h"
#include "shader.h"
#include <GL/glew.h>

#define SHADOW_MAP_SIZE 1024

static const char *vertex_shader = (
# include "shadow.vert.h"
);

static const char *fragment_shader = (
# include "shadow.frag.h"
);

static struct Shader *shader = NULL;
static struct ShaderSource *shader_sources[2] = { NULL, NULL };

static GLuint shadow_map_texture = 0;
static GLuint shadow_map_fb = 0;

void
shadow_pass_cleanup(void)
{
	glDeleteFramebuffers(1, &shadow_map_fb);
	shadow_map_fb = 0;

	glDeleteTextures(1, &shadow_map_texture);
	shadow_map_texture = 0;

	shader_source_free(shader_sources[0]);
	shader_source_free(shader_sources[1]);
	shader_sources[0] = shader_sources[1] = NULL;

	shader_free(shader);
	shader = NULL;
}

int
shadow_pass_init(void)
{
	int ok = 1;

	// compile shadow pass shader
	shader_sources[0] = shader_source_from_string(
		vertex_shader,
		GL_VERTEX_SHADER
	);
	shader_sources[1] = shader_source_from_string(
		fragment_shader,
		GL_FRAGMENT_SHADER
	);
	if (!shader_sources[0] ||
	    !shader_sources[1] ||
	    !(shader = shader_new(shader_sources, 2))) {
		errf(ERR_GENERIC, "failed to compile shadow pass shader");
		goto error;
	}

	// create a texture for shadow map
	glGenTextures(1, &shadow_map_texture);
	if (!shadow_map_texture) {
		err(ERR_OPENGL);
		goto error;
	}

	// configure the texture to have rectangular size and store 16-bit depth
	// values
	glBindTexture(GL_TEXTURE_2D, shadow_map_texture);
	glTexImage2D(
		GL_TEXTURE_2D,
		0,
		GL_DEPTH_COMPONENT16,
		SHADOW_MAP_SIZE,
		SHADOW_MAP_SIZE,
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
	glGenFramebuffers(1, &shadow_map_fb);
	if (!shadow_map_fb || glGetError() != GL_NO_ERROR) {
		err(ERR_OPENGL);
		goto error;
	}
	glBindFramebuffer(GL_FRAMEBUFFER, shadow_map_fb);
	glFramebufferTexture(
		GL_FRAMEBUFFER,
		GL_DEPTH_ATTACHMENT,
		shadow_map_texture,
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
	return ok;

error:
	shadow_pass_cleanup();
	ok = 0;
	goto cleanup;
}

int
shadow_pass_enter(void)
{
	// TODO
	return 0;
}

int
shadow_pass_exit(void)
{
	// TODO
	return 0;
}


struct Shader*
shadow_pass_get_shader(void)
{
	return shader;
}