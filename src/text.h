#pragma once

struct Object;

struct Text {
	// public
	Vec color;
	float opacity;

	// read-only
	size_t len;
	unsigned width;
	unsigned height;

	// private
	struct Font *font;
	struct Buffer *chars;
	struct Buffer *coords;
	struct Geometry *geometry;
};

struct Text*
text_new(struct Font *font);

int
text_set_string(struct Text *text, const char *str);

int
text_set_fmt(struct Text *text, const char *fmt, ...);

struct Object*
text_to_object(struct Text *text);

void
text_free(struct Text *text);
