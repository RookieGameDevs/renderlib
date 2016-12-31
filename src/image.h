#pragma once

struct Image {
	unsigned width, height;
	void *data;
};

struct Image*
image_from_file(const char *filename);

void
image_free(struct Image *image);
