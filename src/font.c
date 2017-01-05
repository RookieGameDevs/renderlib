#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_GLYPH_H

#include "error.h"
#include "file_utils.h"
#include "font.h"
#include <assert.h>
#include <stdlib.h>

static int ft_initialized = 0;
static FT_Library ft;

struct Font {
	struct Character charmap[128];
	GLuint tex_glyph;
	GLuint tex_atlas;
	unsigned tex_atlas_offset;
};

static void
shutdown_freetype(void)
{
	FT_Done_FreeType(ft);
	ft_initialized = 0;
}

static int
init_freetype(void)
{
	if (FT_Init_FreeType(&ft) != 0) {
		return 0;
	}
	atexit(shutdown_freetype);
	ft_initialized = 1;
	return 1;
}

static int
init_glyph_texture(struct Font *font, FT_Glyph *glyphs)
{
	GLubyte data[128][2];
	for (unsigned char c = 0; c < 128; c++) {
		struct Character *ch = &font->charmap[c];
		data[c][0] = ch->size[0];
		data[c][1] = ch->size[1];
	}

	glGenTextures(1, &font->tex_glyph);
	if (!font->tex_glyph) {
		err(ERR_OPENGL);
		return 0;
	}

	glBindTexture(GL_TEXTURE_1D, font->tex_glyph);
	// NOTE: `GL_NEAREST` is *mandatory* with integer textures.
	// http://stackoverflow.com/a/39578132/324790
	glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_BASE_LEVEL, 0);
	glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MAX_LEVEL, 0);
	glTexImage1D(
		GL_TEXTURE_1D,
		0,
		GL_RG16UI,
		128,
		0,
		GL_RG_INTEGER,
		GL_UNSIGNED_BYTE,
		data
	);
	glBindTexture(GL_TEXTURE_1D, 0);

	if (glGetError() != GL_NO_ERROR) {
		glDeleteTextures(1, &font->tex_glyph);
		font->tex_glyph = 0;
		err(ERR_OPENGL);
		return 0;
	}

	return 1;
}

static int
init_atlas_texture(struct Font *font, FT_Glyph *glyphs)
{
	// compute the atlas height and step between glyphs
	unsigned atlas_w = 0, atlas_h = 0, atlas_s = 0;
	for (unsigned c = 0; c < 128; c++) {
		FT_Bitmap *bmp = &((FT_BitmapGlyph)glyphs[c])->bitmap;
		unsigned width = abs(bmp->pitch);
		unsigned height = bmp->rows;
		if (height > atlas_h) {
			atlas_h = height;
		}
		if (width > atlas_s) {
			atlas_s = width;
		}
	}
	font->tex_atlas_offset = atlas_s;
	atlas_w = atlas_s * 128;

	// texture data buffer
	unsigned char data[atlas_w * atlas_h];
	memset(data, 0, sizeof(data));

	// blit each glyph to atlas
	for (unsigned c = 0; c < 128; c++) {
		FT_Bitmap *bmp = &((FT_BitmapGlyph)glyphs[c])->bitmap;
		for (unsigned row = 0; row < bmp->rows; row++) {
			// NOTE: both FreeType and OpenGL assume the origin in
			// lower-left corner, thus, when copying the rows we
			// start from lower one and go up
			memcpy(
				data + (bmp->rows - row - 1) * atlas_w + c * atlas_s,
				bmp->buffer + row * bmp->pitch,
				abs(bmp->pitch)  // NOTE: pitch can be negative
			);
		}
	}

	// create the atlas texture
	glGenTextures(1, &font->tex_atlas);
	if (!font->tex_atlas) {
		err(ERR_OPENGL);
		return 0;
	}

	// setup the texture as single-component with no mipmaps
	glBindTexture(GL_TEXTURE_RECTANGLE, font->tex_atlas);
	glTexParameteri(GL_TEXTURE_RECTANGLE, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_RECTANGLE, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_RECTANGLE, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_RECTANGLE, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_RECTANGLE, GL_TEXTURE_BASE_LEVEL, 0);
	glTexParameteri(GL_TEXTURE_RECTANGLE, GL_TEXTURE_MAX_LEVEL, 0);
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	glTexImage2D(
		GL_TEXTURE_RECTANGLE,
		0,
		GL_R8,
		atlas_w,
		atlas_h,
		0,
		GL_RED,
		GL_UNSIGNED_BYTE,
		data
	);
	glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
	glBindTexture(GL_TEXTURE_RECTANGLE, 0);

	if (glGetError() != GL_NO_ERROR) {
		glDeleteTextures(1, &font->tex_glyph);
		font->tex_glyph = 0;
		err(ERR_OPENGL);
		return 0;
	}

	return 1;
}

static int
init_font(struct Font *font, FT_Face face)
{
	// initialize the charmap table
	memset(font->charmap, 0, sizeof(struct Character) * 128);

	// bitmaps copies
	FT_Glyph glyphs[128];

	for (unsigned char c = 0; c < 128; c++) {
		if (FT_Load_Char(face, c, FT_LOAD_RENDER) != 0 ||
		    FT_Get_Glyph(face->glyph, &glyphs[c]) != 0 ||
		    FT_Glyph_To_Bitmap(&glyphs[c], FT_RENDER_MODE_NORMAL, NULL, 0) != 0) {
			err(ERR_FREETYPE);
			return 0;
		}
		struct Character *ch = &font->charmap[c];
		ch->size[0] = face->glyph->bitmap.width;
		ch->size[1] = face->glyph->bitmap.rows;
		ch->bearing[0] = face->glyph->bitmap_left;
		ch->bearing[1] = face->glyph->bitmap_top;
		ch->advance = face->glyph->advance.x;
	}

	int ok = (
		init_glyph_texture(font, glyphs) &&
		init_atlas_texture(font, glyphs)
	);

	for (unsigned char c = 0; c < 128; c++) {
		FT_Done_Glyph(glyphs[c]);
	}

	return ok;
}

struct Font*
font_from_buffer(const char *data, size_t size, unsigned pt)
{
	assert(data != NULL);
	assert(size > 0);
	assert(pt > 0);

	FT_Face face = NULL;
	struct Font *font = NULL;

	if ((!ft_initialized && !init_freetype())) {
		err(ERR_FREETYPE);
		goto error;
	} else if (FT_New_Memory_Face(ft, (FT_Byte*)data, size, 0, &face) != 0 ||
	           FT_Set_Pixel_Sizes(face, 0, pt) != 0) {
		err(ERR_INVALID_FONT);
		goto error;
	} else if (!(font = malloc(sizeof(struct Font)))) {
		err(ERR_NO_MEM);
		goto error;
	} else if (!init_font(font, face)) {
		goto error;
	}

cleanup:
	if (face) {
		FT_Done_Face(face);
	}

	return font;

error:
	font_free(font);
	font = NULL;
	goto cleanup;
}

struct Font*
font_from_file(const char *filename, unsigned pt)
{
	assert(filename != NULL);
	assert(strlen(filename) > 0);
	assert(pt > 0);

	// attempt to read the font file
	char *data = NULL;
	size_t size = file_read(filename, &data);
	if (!size) {
		return NULL;
	}

	// create font from just read contents
	struct Font *font = font_from_buffer(data, size, pt);

	free(data);

	return font;
}

const struct Character*
font_get_char(struct Font *font, char c)
{
	assert(font != NULL);
	return &font->charmap[(int)c];
}

GLuint
font_get_glyph_texture(struct Font *font)
{
	assert(font != NULL);
	return font->tex_glyph;
}

GLuint
font_get_atlas_texture(struct Font *font)
{
	assert(font != NULL);
	return font->tex_atlas;
}

unsigned
font_get_atlas_offset(struct Font *font)
{
	assert(font != NULL);
	return font->tex_atlas_offset;
}

void
font_free(struct Font *font)
{
	if (font) {
		free(font);
	}
}
