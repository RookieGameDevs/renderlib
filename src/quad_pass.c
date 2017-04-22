#include "error.h"
#include "shader.h"

static const char *vertex_shader = (
# include "quad.vert.h"
);

static const char *fragment_shader = (
# include "quad.frag.h"
);

static struct Shader *shader = NULL;
static struct ShaderSource *shader_sources[2] = { NULL, NULL };

void
quad_pass_cleanup(void)
{
	shader_source_free(shader_sources[0]);
	shader_source_free(shader_sources[1]);
	shader_sources[0] = shader_sources[1] = NULL;

	shader_free(shader);
	shader = NULL;
}

int
quad_pass_init(void)
{
	// compile quad pass shader
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
		errf(ERR_GENERIC, "failed to compile quad pass shader");
		quad_pass_cleanup();
		return 0;
	}

	return 1;
}

int
quad_pass_enter(void)
{
	// TODO
	return 0;
}

int
quad_pass_exit(void)
{
	// TODO
	return 0;
}