// renderlib
#include "renderlib.h"
// standard C library
#include <stdlib.h>

static const char *vertex_shader = (
# include "quad.vert.h"
);

static const char *fragment_shader = (
# include "quad.frag.h"
);

static struct RenderPass*
quad_pass_alloc(void);

static void
quad_pass_free(struct RenderPass *pass);

static int
quad_pass_enter(struct RenderPass *pass);

static int
quad_pass_exit(struct RenderPass *pass);

static struct Shader*
quad_pass_get_shader(struct RenderPass *pass);

struct RenderPassCls quad_pass_cls = {
	.name = "quad",
	.alloc = quad_pass_alloc,
	.free = quad_pass_free,
	.enter = quad_pass_enter,
	.exit = quad_pass_exit,
	.get_shader = quad_pass_get_shader
};

struct QuadPass {
	struct RenderPass super;
	struct Shader *shader;
	struct ShaderSource *shader_sources[2];
};

static struct RenderPass*
quad_pass_alloc(void)
{
	struct QuadPass *pass = malloc(sizeof(struct QuadPass));
	if (!pass) {
		err(ERR_NO_MEM);
		return NULL;
	}
	pass->super.cls = &quad_pass_cls;

	// compile quad pass shader
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
		errf(ERR_GENERIC, "failed to compile quad pass shader");
		quad_pass_free((struct RenderPass*)pass);
		return NULL;
	}

	return (struct RenderPass*)pass;
}

static void
quad_pass_free(struct RenderPass *pass)
{
	if (pass) {
		struct QuadPass *_pass = (struct QuadPass*)pass;
		shader_source_free(_pass->shader_sources[0]);
		shader_source_free(_pass->shader_sources[1]);
		shader_free(_pass->shader);
	}
}

static int
quad_pass_enter(struct RenderPass *pass)
{
	return 1;
}

static int
quad_pass_exit(struct RenderPass *pass)
{
	return 1;
}

static struct Shader*
quad_pass_get_shader(struct RenderPass *pass)
{
	return ((struct QuadPass*)pass)->shader;
}