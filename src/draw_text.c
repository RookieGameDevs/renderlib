#include "renderlib.h"
#include <GL/glew.h>
#include <assert.h>
#include <stdlib.h>

static const char *vertex_shader = (
# include "text.vert.h"
);

static const char *fragment_shader = (
# include "text.frag.h"
);

static struct Shader *shader = NULL;
static struct ShaderSource *shader_sources[2] = { NULL, NULL };
static struct ShaderUniform u_mvp;
static struct ShaderUniform u_glyph_map_sampler;
static struct ShaderUniform u_atlas_map_sampler;
static struct ShaderUniform u_atlas_offset;
static struct ShaderUniform u_color;
static struct ShaderUniform u_opacity;

static void
cleanup(void)
{
	shader_free(shader);
	shader_source_free(shader_sources[0]);
	shader_source_free(shader_sources[1]);
}

int
init_text_pipeline(void)
{
	// cleanup resources at program exit
	atexit(cleanup);

	// uniform names and receiver pointers
	const char *uniform_names[] = {
		"mvp",
		"glyph_map_sampler",
		"atlas_map_sampler",
		"atlas_offset",
		"color",
		"opacity",
		NULL
	};
	struct ShaderUniform *uniforms[] = {
		&u_mvp,
		&u_glyph_map_sampler,
		&u_atlas_map_sampler,
		&u_atlas_offset,
		&u_color,
		&u_opacity
	};

	// compile text pipeline shader and initialize uniforms
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
		errf(ERR_GENERIC, "text pipeline shader compile failed");
		return 0;
	} else if (!shader_get_uniforms(shader, uniform_names, uniforms)) {
		errf(ERR_GENERIC, "bad text pipeline shader");
		return 0;
	}

	// check for any OpenGL-related errors
	if (glGetError() != GL_NO_ERROR) {
		err(ERR_OPENGL);
		return 0;
	}

	return 1;
}

int
draw_text(struct Text *text, struct TextProps *props, struct Transform *transform)
{
	assert(text != NULL);
	assert(props != NULL);

	Mat mv, mvp;
	mat_mul(&transform->view, &transform->model, &mv);
	mat_mul(&transform->projection, &mv, &mvp);

	int glyph_map_sampler = 0, glyph_map = font_get_glyph_texture(text->font);
	glActiveTexture(GL_TEXTURE0 + glyph_map_sampler);
	glBindTexture(GL_TEXTURE_1D, glyph_map);

	int atlas_map_sampler = 1, atlas_map = font_get_atlas_texture(text->font);
	glActiveTexture(GL_TEXTURE0 + atlas_map_sampler);
	glBindTexture(GL_TEXTURE_RECTANGLE, atlas_map);

	if (glGetError() != GL_NO_ERROR) {
		glBindTexture(GL_TEXTURE_1D, 0);
		glBindTexture(GL_TEXTURE_RECTANGLE, 0);
		err(ERR_OPENGL);
		return 0;
	}

	int atlas_offset = font_get_atlas_offset(text->font);

	int configured = (
		shader_bind(shader) &&
		shader_uniform_set(&u_mvp, 1, &mvp) &&
		shader_uniform_set(&u_glyph_map_sampler, 1, &glyph_map_sampler) &&
		shader_uniform_set(&u_atlas_map_sampler, 1, &atlas_map_sampler) &&
		shader_uniform_set(&u_atlas_offset, 1, &atlas_offset) &&
		shader_uniform_set(&u_color, 1, &props->color) &&
		shader_uniform_set(&u_opacity, 1, &props->opacity)
	);
	if (!configured) {
		errf(ERR_GENERIC, "failed to configure text pipeline");
		return 0;
	}

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glBindVertexArray(text->vao);
        glDrawArraysInstanced(GL_TRIANGLE_STRIP, 0, 4, text->len);
	glDisable(GL_BLEND);

#ifdef DEBUG
	if (glGetError() != GL_NO_ERROR) {
		err(ERR_OPENGL);
		return 0;
	}
#endif
	return 1;
}
