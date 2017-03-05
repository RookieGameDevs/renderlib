#include "renderlib.h"
#include <GL/glew.h>
#include <assert.h>
#include <stdlib.h>

static const char *vertex_shader = (
# include "quad.vert.h"
);

static const char *fragment_shader = (
# include "quad.frag.h"
);

static struct Shader *shader = NULL;
static struct ShaderSource *shader_sources[2] = { NULL, NULL };
static struct ShaderUniform u_mvp;
static struct ShaderUniform u_size;
static struct ShaderUniform u_border;
static struct ShaderUniform u_color;
static struct ShaderUniform u_opacity;
static struct ShaderUniform u_texture_sampler;
static struct ShaderUniform u_enable_texture_mapping;

static GLuint quad_vao = 0;

static void
cleanup(void)
{
	glDeleteVertexArrays(1, &quad_vao);
	shader_free(shader);
	shader_source_free(shader_sources[0]);
	shader_source_free(shader_sources[1]);
}

int
init_quad_pipeline(void)
{
	// cleanup resources at program exit
	atexit(cleanup);

	// uniform names and receiver pointers
	const char *uniform_names[] = {
		"mvp",
		"size",
		"border",
		"color",
		"opacity",
		"texture_sampler",
		"enable_texture_mapping",
		NULL
	};
	struct ShaderUniform *uniforms[] = {
		&u_mvp,
		&u_size,
		&u_border,
		&u_color,
		&u_opacity,
		&u_texture_sampler,
		&u_enable_texture_mapping
	};

	// compile quad pipeline shader and initialize uniforms
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
		errf(ERR_GENERIC, "quad pipeline shader compile failed");
		return 0;
	} else if (!shader_get_uniforms(shader, uniform_names, uniforms)) {
		errf(ERR_GENERIC, "bad quad pipeline shader");
		return 0;
	}

	// create a VAO for the quad prototype
	glGenVertexArrays(1, &quad_vao);

	// check for any OpenGL-related errors
	if (glGetError() != GL_NO_ERROR) {
		err(ERR_OPENGL);
		return 0;
	}

	return 1;
}

int
draw_quad(struct Quad *quad, struct QuadProps *props, struct Transform *transform)
{
	assert(quad != NULL);
	assert(props != NULL);
	assert(transform != NULL);

	Mat mv, mvp;
	mat_mul(&transform->view, &transform->model, &mv);
	mat_mul(&transform->projection, &mv, &mvp);

	int enable_texture_mapping = props->texture != NULL;
	int texture_sampler = 0;
	if (enable_texture_mapping) {
		glActiveTexture(GL_TEXTURE0 + texture_sampler);
		glBindTexture(GL_TEXTURE_RECTANGLE, props->texture->id);
		if (glGetError() != GL_NO_ERROR) {
			err(ERR_OPENGL);
			return 0;
		}
	}

	Vec size = vec(quad->width, quad->height, 0, 0);
	Vec borders = vec(
		props->borders.left,
		props->borders.right,
		props->borders.top,
		props->borders.bottom
	);

	int configured = (
		shader_bind(shader) &&
		shader_uniform_set(&u_mvp, 1, &mvp) &&
		shader_uniform_set(&u_size, 1, &size) &&
		shader_uniform_set(&u_border, 1, &borders) &&
		shader_uniform_set(&u_color, 1, &props->color) &&
		shader_uniform_set(&u_opacity, 1, &props->opacity) &&
		shader_uniform_set(&u_enable_texture_mapping, 1, &enable_texture_mapping) &&
		shader_uniform_set(&u_texture_sampler, 1, &texture_sampler)
	);
	if (!configured) {
		errf(ERR_GENERIC, "failed to configure quad pipeline");
		return 0;
	}

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glBindVertexArray(quad_vao);
        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
	glDisable(GL_BLEND);

#ifdef DEBUG
	if (glGetError() != GL_NO_ERROR) {
		err(ERR_OPENGL);
		return 0;
	}
#endif
	return 1;
}
