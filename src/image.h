#pragma once

enum {
	IMAGE_FORMAT_RGBA = 1,
	IMAGE_FORMAT_RGB
};

struct Image {
	unsigned width, height;
	int format;
	void *data;
};

struct Image*
image_from_file(const char *filename);

void
image_free(struct Image *image);
