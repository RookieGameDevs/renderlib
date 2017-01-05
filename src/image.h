#pragma once

#include <stddef.h>

enum {
	IMAGE_FORMAT_RGBA = 1,
	IMAGE_FORMAT_RGB
};

enum {
	IMAGE_CODEC_PNG = 1,
	IMAGE_CODEC_JPEG,
};

struct Image {
	unsigned width, height;
	int format;
	void *data;
};

struct Image*
image_from_buffer(const char *data, size_t size, int codec);

struct Image*
image_from_file(const char *filename);

void
image_free(struct Image *image);
