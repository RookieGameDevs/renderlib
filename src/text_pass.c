// renderlib
#include "renderlib.h"
#include "text_pass.h"
// standard C library
#include <assert.h>
#include <stdlib.h>
#include <string.h>

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

static struct RenderPassUniformSet*
text_pass_create_uniform_set(struct RenderPass *pass);

static void
text_pass_uniform_set_free(struct RenderPassUniformSet *set);

static struct ShaderUniformValue*
text_pass_uniform_set_get_value(struct RenderPassUniformSet *set, int id);

struct ShaderUniformValue*
text_pass_uniform_set_get_values(struct RenderPassUniformSet *s, unsigned *r_count);

struct RenderPassCls text_pass_cls = {
	.name = "text",
	.alloc = text_pass_alloc,
	.free = text_pass_free,
	.enter = text_pass_enter,
	.exit = text_pass_exit,
	.get_shader = text_pass_get_shader,
	.create_uniform_set = text_pass_create_uniform_set
};

struct TextPass {
	struct RenderPass super;
	struct Shader *shader;
	struct ShaderSource *shader_sources[2];
};

static struct RenderPassUniformSetCls text_pass_uniform_set_cls = {
	.free = text_pass_uniform_set_free,
	.get_value = text_pass_uniform_set_get_value,
	.get_values = text_pass_uniform_set_get_values
};

struct TextPassUniformSet {
	struct RenderPassUniformSet super;
	struct ShaderUniformValue values[TEXT_PASS_UNIFORM_COUNT];
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
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	return 1;
}

static int
text_pass_exit(struct RenderPass *pass)
{
	glDisable(GL_BLEND);
	return 1;
}

static struct Shader*
text_pass_get_shader(struct RenderPass *pass)
{
	return ((struct TextPass*)pass)->shader;
}

static struct RenderPassUniformSet*
text_pass_create_uniform_set(struct RenderPass *pass)
{
	// allocate uniform set container
	struct TextPassUniformSet *set = malloc(sizeof(struct TextPassUniformSet));
	if (!set) {
		err(ERR_NO_MEM);
		return NULL;
	}
	set->super.cls = &text_pass_uniform_set_cls;

	struct Shader *shader = pass->cls->get_shader(pass);

	static const char *uniform_names[] = {
		"mvp",
		"glyph_map_sampler",
		"atlas_map_sampler",
		"atlas_offset",
		"color",
		"opacity",
	};
	for (unsigned i = 0; i < TEXT_PASS_UNIFORM_COUNT; i++) {
		int found = 0;
		for (unsigned u = 0; u < shader->uniform_count; u++) {
			struct ShaderUniform *uniform = &shader->uniforms[u];
			if (strcmp(uniform_names[i], uniform->name) == 0) {
				set->values[i].uniform = uniform;
				set->values[i].data = NULL;
				set->values[i].count = 0;
			}
			found = 1;
		}
		assert(found);
	}

	return (struct RenderPassUniformSet*)set;
}

static void
text_pass_uniform_set_free(struct RenderPassUniformSet *set)
{
	free(set);
}

static struct ShaderUniformValue*
text_pass_uniform_set_get_value(struct RenderPassUniformSet *set, int id)
{
	assert(id < TEXT_PASS_UNIFORM_COUNT);
	return &((struct TextPassUniformSet*)set)->values[id];
}

struct ShaderUniformValue*
text_pass_uniform_set_get_values(struct RenderPassUniformSet *set, unsigned *r_count)
{
	*r_count = TEXT_PASS_UNIFORM_COUNT;
	return ((struct TextPassUniformSet*)set)->values;
}