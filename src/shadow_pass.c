#include "error.h"
#include "shader.h"
#include "renderlib.h"
#include <GL/glew.h>
#include <stdlib.h>

#define SHADOW_MAP_SIZE 1024

static const char *vertex_shader = (
# include "shadow.vert.h"
);

static const char *fragment_shader = (
# include "shadow.frag.h"
);

static struct RenderPass*
shadow_pass_alloc(void);

static void
shadow_pass_free(struct RenderPass *pass);

static int
shadow_pass_enter(struct RenderPass *pass);

static int
shadow_pass_exit(struct RenderPass *pass);

static struct Shader*
shadow_pass_get_shader(struct RenderPass *pass);

struct RenderPassCls shadow_pass_cls = {
	.name = "shadow",
	.alloc = shadow_pass_alloc,
	.free = shadow_pass_free,
	.enter = shadow_pass_enter,
	.exit = shadow_pass_exit,
	.get_shader = shadow_pass_get_shader
};

struct ShadowPass {
	struct RenderPass super;
	GLuint shadow_map_texture;
	GLuint shadow_map_fb;
	GLint viewport[4];
	struct Shader *shader;
	struct ShaderSource *shader_sources[2];
};


static int
shadow_pass_enter(struct RenderPass *pass)
{
	struct ShadowPass *_pass = (struct ShadowPass*)pass;

	// get current viewport dimensions
	glGetIntegerv(GL_VIEWPORT, _pass->viewport);

	// set viewport to the size of the shadow map texture
	glViewport(0, 0, SHADOW_MAP_SIZE, SHADOW_MAP_SIZE);

	// bind the shadow map framebuffer
	glBindFramebuffer(GL_FRAMEBUFFER, _pass->shadow_map_fb);
	glClear(GL_DEPTH_BUFFER_BIT);

	return glGetError() == GL_NO_ERROR;
}

static int
shadow_pass_exit(struct RenderPass *pass)
{
	struct ShadowPass *_pass = (struct ShadowPass*)pass;

	// restore the default framebuffer
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	// restore original viewport size
	glViewport(
		_pass->viewport[0],
		_pass->viewport[1],
		_pass->viewport[2],
		_pass->viewport[3]
	);

	return glGetError() == GL_NO_ERROR;
}


static struct Shader*
shadow_pass_get_shader(struct RenderPass *pass)
{
	return ((struct ShadowPass*)pass)->shader;;
}

static void
shadow_pass_free(struct RenderPass *pass)
{
	if (pass) {
		struct ShadowPass *_pass = (struct ShadowPass*)pass;
		glDeleteFramebuffers(1, &_pass->shadow_map_fb);
		glDeleteTextures(1, &_pass->shadow_map_texture);
		shader_source_free(_pass->shader_sources[0]);
		shader_source_free(_pass->shader_sources[1]);
		shader_free(_pass->shader);
	}
}

static struct RenderPass*
shadow_pass_alloc(void)
{
	struct ShadowPass *pass = malloc(sizeof(struct ShadowPass));
	if (!pass) {
		err(ERR_NO_MEM);
		return NULL;
	}
	pass->super.cls = &shadow_pass_cls;

	// compile shadow pass shader
	pass->shader_sources[0] = shader_source_from_string(
		vertex_shader,
		GL_VERTEX_SHADER
	);
	pass->shader_sources[1] = shader_source_from_string(
		fragment_shader,
		GL_FRAGMENT_SHADER
	);
	if (!pass->shader_sources[0] ||
	    !pass->shader_sources[1] ||
	    !(pass->shader = shader_new(pass->shader_sources, 2))) {
		errf(ERR_GENERIC, "failed to compile shadow pass shader");
		goto error;
	}

	// create a texture for shadow map
	glGenTextures(1, &pass->shadow_map_texture);
	if (!pass->shadow_map_texture) {
		err(ERR_OPENGL);
		goto error;
	}

	// configure the texture to have rectangular size and store 16-bit depth
	// values
	glBindTexture(GL_TEXTURE_2D, pass->shadow_map_texture);
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
	glGenFramebuffers(1, &pass->shadow_map_fb);
	if (!pass->shadow_map_fb || glGetError() != GL_NO_ERROR) {
		err(ERR_OPENGL);
		goto error;
	}
	glBindFramebuffer(GL_FRAMEBUFFER, pass->shadow_map_fb);
	glFramebufferTexture(
		GL_FRAMEBUFFER,
		GL_DEPTH_ATTACHMENT,
		pass->shadow_map_texture,
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
	return (struct RenderPass*)pass;

error:
	shadow_pass_free((struct RenderPass*)pass);
	pass = NULL;
	goto cleanup;
}