#include "error.h"
#include "font.h"
#include "text.h"
#include <GL/glew.h>
#include <assert.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct Text*
text_new(struct Font *font)
{
	assert(font != NULL);

	struct Text *text = malloc(sizeof(struct Text));
	if (!text) {
		err(ERR_NO_MEM);
		return NULL;
	}
	text->len = 0;
	text->width = 0;
	text->height = 0;
	text->vao = text->coords = text->chars = 0;
	text->font = font;

	// generate vertex array
	glGenVertexArrays(1, &text->vao);
	glBindVertexArray(text->vao);

	// generate vertex buffers
	// NOTE: buffers are initialized when `text_set_string()` is called
	glGenBuffers(1, &text->coords);
	glGenBuffers(1, &text->chars);

	// setup coords attribute array
	glBindBuffer(GL_ARRAY_BUFFER, text->coords);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, (GLvoid*)0);
	glVertexAttribDivisor(0, 1);

	// setup character indices attribute array
	// NOTE: each index is one byte long (the character itself), thus,
	// anything except ASCII is not supported
	glBindBuffer(GL_ARRAY_BUFFER, text->chars);
	glEnableVertexAttribArray(1);
	glVertexAttribIPointer(1, 1, GL_UNSIGNED_BYTE, 0, (GLvoid*)0);
	glVertexAttribDivisor(1, 1);

	if (!text->vao || !text->chars || !text->coords ||
	    glGetError() != GL_NO_ERROR) {
		err(ERR_OPENGL);
		text_free(text);
		text = NULL;
	}

	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);

	return text;
}

int
text_set_string(struct Text *text, const char *str)
{
	assert(text != NULL);
	assert(str != NULL);

	text->len = strlen(str);

	// setup the buffer of character indices, which in turn are character
	// themselves
	// NOTE: this doesn't support anything except ASCII
	glBindBuffer(GL_ARRAY_BUFFER, text->chars);
	glBufferData(GL_ARRAY_BUFFER, text->len, str, GL_STATIC_DRAW);
	if (glGetError() != GL_NO_ERROR) {
		glBindBuffer(GL_ARRAY_BUFFER, 0);
		err(ERR_OPENGL);
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

	// update the character coordinates buffer
	glBindBuffer(GL_ARRAY_BUFFER, text->coords);
	glBufferData(
		GL_ARRAY_BUFFER,
		sizeof(float) * text->len * 2,
		coords,
		GL_STATIC_DRAW
	);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	if (glGetError() != GL_NO_ERROR) {
		glBindBuffer(GL_ARRAY_BUFFER, 0);
		err(ERR_OPENGL);
		return 0;
	}

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
		glDeleteBuffers(1, &text->chars);
		glDeleteBuffers(1, &text->coords);
		glDeleteVertexArrays(1, &text->vao);
		free(text);
	}
}
