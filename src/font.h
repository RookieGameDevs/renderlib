#pragma once

#include <GL/glew.h>

struct Character {
	unsigned short size[2];
	int bearing[2];
	unsigned advance;
};

struct Font;

struct Font*
font_from_file(const char *filename, unsigned size);

const struct Character*
font_get_char(struct Font *font, char c);

GLuint
font_get_glyph_texture(struct Font *font);

GLuint
font_get_atlas_texture(struct Font *font);

unsigned
font_get_atlas_offset(struct Font *font);

void
font_free(struct Font *font);
