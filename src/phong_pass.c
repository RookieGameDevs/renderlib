// renderlib
#include "renderlib.h"
// standard C library
#include <stdlib.h>

static const char *vertex_shader = (
# include "phong.vert.h"
);

static const char *fragment_shader = (
# include "phong.frag.h"
);

static struct RenderPass*
phong_pass_alloc(void);

static void
phong_pass_free(struct RenderPass *pass);

static int
phong_pass_enter(struct RenderPass *pass);

static int
phong_pass_exit(struct RenderPass *pass);

static struct Shader*
phong_pass_get_shader(struct RenderPass *pass);

struct RenderPassCls phong_pass_cls = {
	.name = "phong",
	.alloc = phong_pass_alloc,
	.free = phong_pass_free,
	.enter = phong_pass_enter,
	.exit = phong_pass_exit,
	.get_shader = phong_pass_get_shader
};

struct PhongPass {
	struct RenderPass super;
	struct Shader *shader;
	struct ShaderSource *shader_sources[2];
};

static struct RenderPass*
phong_pass_alloc(void)
{
	struct PhongPass *pass = malloc(sizeof(struct PhongPass));
	if (!pass) {
		err(ERR_NO_MEM);
		return NULL;
	}
	pass->super.cls = &phong_pass_cls;

	// compile phong pass shader
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
		errf(ERR_GENERIC, "failed to compile phong pass shader");
		phong_pass_free((struct RenderPass*)pass);
		return NULL;
	}

	return (struct RenderPass*)pass;
}

static void
phong_pass_free(struct RenderPass *pass)
{
	if (pass) {
		struct PhongPass *_pass = (struct PhongPass*)pass;
		shader_source_free(_pass->shader_sources[0]);
		shader_source_free(_pass->shader_sources[1]);
		shader_free(_pass->shader);
	}
}

static int
phong_pass_enter(struct RenderPass *pass)
{
	return 1;
}

static int
phong_pass_exit(struct RenderPass *pass)
{
	return 1;
}

static struct Shader*
phong_pass_get_shader(struct RenderPass *pass)
{
	return ((struct PhongPass*)pass)->shader;
}