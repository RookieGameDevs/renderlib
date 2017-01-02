#include "renderlib.h"
#include <GL/glew.h>
#include <assert.h>
#include <stdlib.h>

static struct Shader *shader = NULL;
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

	// compile mesh pipeline shader and initialize uniforms
	shader = shader_compile(
		"src/shaders/text.vert",
		"src/shaders/text.frag",
		uniform_names,
		uniforms,
		NULL,
		NULL
	);
	if (!shader) {
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
draw_text(struct Text *text, struct TextRenderProps *props)
{
	assert(text != NULL);
	assert(props != NULL);

	Mat mv, mvp;
	mat_mul(&props->view, &props->model, &mv);
	mat_mul(&props->projection, &mv, &mvp);

	int glyph_map_sampler = 0, glyph_map = font_get_glyph_texture(text->font);
	glActiveTexture(GL_TEXTURE0 + glyph_map_sampler);
	glBindTexture(GL_TEXTURE_1D, glyph_map);

	int atlas_map_sampler = 1, atlas_map = font_get_atlas_texture(text->font);
	glActiveTexture(GL_TEXTURE0 + atlas_map_sampler);
	glBindTexture(GL_TEXTURE_RECTANGLE, atlas_map);

	if (glGetError() != GL_NO_ERROR) {
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
		errf(ERR_GENERIC, "failed to configure text pipeline", 0);
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