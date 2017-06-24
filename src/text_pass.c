#include "error.h"
#include "shader.h"
#include "renderlib.h"
#include <stdlib.h>

static const char *vertex_shader = (
# include "text.vert.h"
);

static const char *fragment_shader = (
# include "text.frag.h"
);

static struct RenderPass*
text_pass_alloc(void);

static void
text_pass_free(struct RenderPass *pass);

static int
text_pass_enter(struct RenderPass *pass);

static int
text_pass_exit(struct RenderPass *pass);

static struct Shader*
text_pass_get_shader(struct RenderPass *pass);

struct RenderPassCls text_pass_cls = {
	.name = "text",
	.alloc = text_pass_alloc,
	.free = text_pass_free,
	.enter = text_pass_enter,
	.exit = text_pass_exit,
	.get_shader = text_pass_get_shader
};

struct TextPass {
	struct RenderPass super;
	struct Shader *shader;
	struct ShaderSource *shader_sources[2];
};

static struct RenderPass*
text_pass_alloc(void)
{
	struct TextPass *pass = malloc(sizeof(struct TextPass));
	if (!pass) {
		err(ERR_NO_MEM);
		return NULL;
	}
	pass->super.cls = &text_pass_cls;

	// compile text pass shader
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
		errf(ERR_GENERIC, "failed to compile text pass shader");
		text_pass_free((struct RenderPass*)pass);
		return NULL;
	}

	return (struct RenderPass*)pass;
}

static void
text_pass_free(struct RenderPass *pass)
{
	if (pass) {
		struct TextPass *_pass = (struct TextPass*)pass;
		shader_source_free(_pass->shader_sources[0]);
		shader_source_free(_pass->shader_sources[1]);
		shader_free(_pass->shader);
	}
}


static int
text_pass_enter(struct RenderPass *pass)
{
	// TODO
	return 1;
}

static int
text_pass_exit(struct RenderPass *pass)
{
	// TODO
	return 1;
}

static struct Shader*
text_pass_get_shader(struct RenderPass *pass)
{
	return ((struct TextPass*)pass)->shader;
}