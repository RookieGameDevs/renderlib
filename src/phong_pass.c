#include "error.h"
#include "shader.h"

static const char *vertex_shader = (
# include "phong.vert.h"
);

static const char *fragment_shader = (
# include "phong.frag.h"
);

static struct Shader *shader = NULL;
static struct ShaderSource *shader_sources[2] = { NULL, NULL };

void
phong_pass_cleanup(void)
{
	shader_source_free(shader_sources[0]);
	shader_source_free(shader_sources[1]);
	shader_sources[0] = shader_sources[1] = NULL;

	shader_free(shader);
	shader = NULL;
}

int
phong_pass_init(void)
{
	// compile phong pass shader
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
		errf(ERR_GENERIC, "failed to compile phong pass shader");
		phong_pass_cleanup();
		return 0;
	}

	return 1;
}

int
phong_pass_enter(void)
{
	return 1;
}

int
phong_pass_exit(void)
{
	return 1;
}


struct Shader*
phong_pass_get_shader(void)
{
	return shader;
}