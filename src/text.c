// renderlib
#include "buffer.h"
#include "error.h"
#include "font.h"
#include "geometry.h"
#include "scene.h"
#include "text.h"
#include "text_pass.h"
#include "renderlib.h"
// standard C library
#include <assert.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
// OpenGL extension wrangler
#include <GL/glew.h>

struct Text*
text_new(struct Font *font)
{
	assert(font != NULL);

	struct Text *text = malloc(sizeof(struct Text));
	if (!text) {
		err(ERR_NO_MEM);
		return NULL;
	}
	memset(text, 0, sizeof(struct Text));
	text->font = font;
	text->opacity = 1;
	text->color = vec(1, 1, 1, 1);

	// create attribute buffers
	text->chars = buffer_new(0, NULL, GL_DYNAMIC_DRAW);
	text->coords = buffer_new(0, NULL, GL_DYNAMIC_DRAW);
	if (!text->chars || !text->coords) {
		text_free(text);
		return NULL;
	}

	// create geometry
	text->geometry = geometry_new();
	if (!text->geometry) {
		text_free(text);
		return NULL;
	}

	// setup position attribute array
	int ok = geometry_add_attribute(
		text->geometry,
		text->coords,
		"position",
		GL_FLOAT,
		2,
		sizeof(GLfloat) * 2,
		(void*)0,
		1
	);

	// setup character indices attribute array
	// NOTE: each index is one byte long (the character itself), thus,
	// anything except ASCII is not supported
	ok &= geometry_add_attribute(
		text->geometry,
		text->chars,
		"character",
		GL_UNSIGNED_BYTE,
		1,
		1,
		(void*)0,
		1
	);

	if (!ok) {
		text_free(text);
		return NULL;
	}

	return text;
}

int
text_set_string(struct Text *text, const char *str)
{
	assert(text != NULL);
	assert(str != NULL);

	text->len = strlen(str);

	// update characters buffer
	if (!buffer_update(text->chars, text->len, (void*)str)) {
		return 0;
	}

	// compute character coords relative to the baseline
	GLfloat coords[text->len][2];
	text->width = text->height = 0;
	for (size_t c = 0; c < text->len; c++) {
		const struct Character *ch = font_get_char(
			text->font,
			str[c]
		);
		coords[c][0] = text->width;
		coords[c][1] = ch->bearing[1] - ch->size[1];
		text->width += ch->advance / 64.0;
		text->height = ch->size[1] > text->height ? ch->size[1] : text->height;
	}

	// align the Y coord so that all glyphs have negative coordinates
	int offset = 0;
	for (size_t c = 0; c < text->len; c++) {
		int c_offset = text->height - coords[c][1];
		if (abs(c_offset) > abs(offset)) {
			offset = c_offset;
		}
	}
	for (size_t c = 0; c < text->len; c++) {
		coords[c][1] -= offset;
	}

	// update coordinates buffer
	if (!buffer_update(text->coords, text->len * sizeof(GLfloat) * 2, coords)) {
		return 0;
	}

	// update geometry instanced primitive draw parameters
	geometry_set_instanced_array(text->geometry, 4, text->len);

	return 1;
}

int
text_set_fmt(struct Text *text, const char *fmt, ...)
{
	va_list ap, ap_copy;
	va_start(ap, fmt);

	va_copy(ap_copy, ap);
	size_t len = vsnprintf(NULL, 0, fmt, ap_copy) + 1;
	va_end(ap_copy);
	char str[len + 1];
	str[len] = 0;

	int ok = 1;
	va_copy(ap_copy, ap);
	if (vsnprintf(str, len, fmt, ap)) {
		ok = text_set_string(text, str);
	}
	va_end(ap_copy);

	va_end(ap);
	return ok;
}

void
text_free(struct Text *text)
{
	if (text) {
		buffer_free(text->coords);
		buffer_free(text->chars);
		geometry_free(text->geometry);
		free(text);
	}
}

static void
text_object_free(struct Object *obj);

static int
text_object_render(struct Object *obj, struct ObjectRenderContext *ctx);

struct TextObject {
	struct Object super;
	struct Text *text;
	struct RenderPassUniformSet *uniform_set;
	Mat mvp;
	GLuint glyph_sampler;
	GLuint atlas_sampler;
	GLint atlas_offset;
};

static struct ObjectCls text_object_cls = {
	.name = "text",
	.free = text_object_free,
	.render = text_object_render
};

struct Object*
text_to_object(struct Text *text)
{
	// create and initialize text object
	struct TextObject *obj = malloc(sizeof(struct TextObject));
	if (!obj) {
		err(ERR_NO_MEM);
		return NULL;
	}
	memset(obj, 0, sizeof(struct TextObject));
	obj->text = text;
	obj->super.cls = &text_object_cls;
	obj->glyph_sampler = 0;
	obj->atlas_sampler = 1;
	obj->atlas_offset = font_get_atlas_offset(text->font);
	obj->super.visible = 1;
	obj->super.scale = vec(1, 1, 1, 0);
	obj->super.rotation = qtr(1, 0, 0, 0);
	mat_ident(&obj->mvp);

	// create and initialize uniform set
	struct RenderPass *pass = renderer_get_pass(TEXT_PASS);
	obj->uniform_set = pass->cls->create_uniform_set(pass);
	if (!obj->uniform_set) {
		free(obj);
		return NULL;
	}

#define set_value(_id, _count, _data) \
	{ \
		struct ShaderUniformValue *v = obj->uniform_set->cls->get_value(obj->uniform_set, _id); \
		v->count = _count; \
		v->data = _data; \
	}

	set_value(TEXT_PASS_UNIFORM_MVP, 1, &obj->mvp);
	set_value(TEXT_PASS_UNIFORM_GLYPH_SAMPLER, 1, &obj->glyph_sampler);
	set_value(TEXT_PASS_UNIFORM_ATLAS_SAMPLER, 1, &obj->atlas_sampler);
	set_value(TEXT_PASS_UNIFORM_ATLAS_OFFSET, 1, &obj->atlas_offset);
	set_value(TEXT_PASS_UNIFORM_COLOR, 1, &text->color);
	set_value(TEXT_PASS_UNIFORM_OPACITY, 1, &text->opacity);

#undef set_value

	return (struct Object*)obj;
}

static void
text_object_free(struct Object *obj)
{
	if (obj) {
		struct TextObject *_obj = (struct TextObject*)obj;
		_obj->uniform_set->cls->free(_obj->uniform_set);
		free(_obj);
	}
}

static int
text_object_render_command_pre_exec(void *userdata)
{
	struct TextObject *obj = userdata;
	glActiveTexture(GL_TEXTURE0 + obj->glyph_sampler);
	glBindTexture(GL_TEXTURE_1D, font_get_glyph_texture(obj->text->font));
	glActiveTexture(GL_TEXTURE0 + obj->atlas_sampler);
	glBindTexture(GL_TEXTURE_RECTANGLE, font_get_atlas_texture(obj->text->font));
	return glGetError() == GL_NO_ERROR;
}

static int
text_object_render(struct Object *obj, struct ObjectRenderContext *ctx)
{
	struct TextObject *_obj = (struct TextObject*)obj;

	// compose model-view-projection matrix
	Mat modelview;
	mat_mul(ctx->view, ctx->model, &modelview);
	mat_mul(ctx->projection, &modelview, &_obj->mvp);

	struct ShaderUniformValue *values = NULL;
	unsigned values_count = 0;
	values = _obj->uniform_set->cls->get_values(_obj->uniform_set, &values_count);

	// TODO: move this to appropriate place
	glActiveTexture(GL_TEXTURE0 + _obj->glyph_sampler);
	glBindTexture(GL_TEXTURE_1D, font_get_glyph_texture(_obj->text->font));
	glActiveTexture(GL_TEXTURE0 + _obj->atlas_sampler);
	glBindTexture(GL_TEXTURE_RECTANGLE, font_get_atlas_texture(_obj->text->font));

	struct RenderCommand cmd = {
		.geometry = _obj->text->geometry,
		.primitive_type = GL_TRIANGLE_STRIP,
		.pass = TEXT_PASS,
		.values = values,
		.values_count = values_count,
		.pre_exec = text_object_render_command_pre_exec,
		.post_exec = NULL,
		.userdata = _obj
	};

	return renderer_add_command(&cmd);
}