#pragma once

#include <GL/glew.h>

struct Text {
	size_t len;
	GLuint vao;
	GLuint coords;
	GLuint chars;
	unsigned width;
	unsigned height;
	struct Font *font;
};

struct Text*
text_new(struct Font *font);

int
text_set_string(struct Text *text, const char *str);

int
text_set_fmt(struct Text *text, const char *fmt, ...);

void
text_free(struct Text *text);
