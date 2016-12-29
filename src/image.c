#include "image.h"
#include <assert.h>
#include <png.h>
#include <setjmp.h>
#include <stdlib.h>

static void*
read_png(
	const char *filename,
	unsigned *r_width,
	unsigned *r_height,
	err_t *r_err
) {
	assert(filename != NULL);
	err_t err = 0;

	void *data = NULL;
	png_structp png_ptr = NULL;
	png_infop info_ptr = NULL;
	png_bytepp rows = NULL;

	size_t hdr_size = 8;
	unsigned char hdr[hdr_size];

	// open the given file
	FILE *fp = fopen(filename, "r");
	if (!fp) {
		err = ERR_NO_FILE;
		goto error;
	}

	// attempt to read 8 bytes and check whether we're reading a PNG file
	if (fread(hdr, 1, hdr_size, fp) != hdr_size ||
	    png_sig_cmp(hdr, 0, hdr_size) != 0) {
		err = ERR_INVALID_IMAGE;
		goto error;
	}

	// allocate libpng structs
	png_ptr = png_create_read_struct(
		PNG_LIBPNG_VER_STRING,
		NULL,
		NULL,
		NULL
	);
	if (!png_ptr) {
		err = ERR_LIBPNG;
		goto error;
	}

	info_ptr = png_create_info_struct(png_ptr);
	if (!info_ptr) {
		err = ERR_LIBPNG;
		goto error;
	}

	// set the error handling longjmp point
	if (setjmp(png_jmpbuf(png_ptr))) {
		err = ERR_INVALID_IMAGE;
		goto error;
	}

	// init file reading IO
	png_init_io(png_ptr, fp);
	png_set_sig_bytes(png_ptr, hdr_size);

	// read image information
	png_read_info(png_ptr, info_ptr);

	// get image info
	int color_type = png_get_color_type(png_ptr, info_ptr);
	int bit_depth = png_get_bit_depth(png_ptr, info_ptr);

	// transform paletted images to RGB
	if (color_type == PNG_COLOR_TYPE_PALETTE) {
		png_set_palette_to_rgb(png_ptr);
	}

	// transform packed grayscale images to 8bit
	if (color_type == PNG_COLOR_TYPE_GRAY && bit_depth < 8) {
#if PNG_LIBPNG_VER >= 10400
		png_set_expand_gray_1_2_4_to_8(png_ptr);
#else
		png_set_gray_1_2_4_to_8(png_ptr);
#endif
	} else if (color_type == PNG_COLOR_TYPE_GRAY ||
	           color_type == PNG_COLOR_TYPE_GRAY_ALPHA) {
		png_set_gray_to_rgb(png_ptr);
	}

	// strip 16bit images down to 8bit
	else if (bit_depth == 16) {
		png_set_strip_16(png_ptr);
	}
	// expand 1-byte packed pixels
	else if (bit_depth < 8) {
		png_set_packing(png_ptr);
	}

	// add full alpha channel
	if (png_get_valid(png_ptr, info_ptr, PNG_INFO_tRNS)) {
		png_set_tRNS_to_alpha(png_ptr);
	}

	// retrieve image size
	int width = png_get_image_width(png_ptr, info_ptr);
	int height = png_get_image_height(png_ptr, info_ptr);
	*r_width = width;
	*r_height = height;

	// allocate space for image data
	size_t rowbytes = png_get_rowbytes(png_ptr, info_ptr);
	data = malloc(height * rowbytes);
	if (!data) {
		err = ERR_NO_MEM;
		goto error;
	}

	// setup an array of image row pointers
	rows = malloc(height * sizeof(png_bytep));
	if (!rows) {
		err = ERR_NO_MEM;
		goto error;
	}
	for (size_t r = 0; r < height; r++) {
		rows[r] = data + rowbytes * r;
	}

	// read image data
	png_read_image(png_ptr, rows);

cleanup:
	free(rows);
	png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
	if (fp) {
		fclose(fp);
	}
	return data;

error:
	if (r_err) {
		*r_err = err;
	}
	free(data);
	data = NULL;
	goto cleanup;
}

struct Image*
image_from_file(const char *filename, err_t *r_err)
{
	assert(filename != NULL);

	struct Image *image = NULL;
	err_t err = 0;

	// allocate image struct
	if (!(image = malloc(sizeof(struct Image)))) {
		err = ERR_NO_MEM;
		goto error;
	}

	// read PNG image
	image->data = read_png(filename, &image->width, &image->height, &err);
	if (!image->data) {
		goto error;
	}

	return image;

error:
	if (r_err) {
		*r_err = err;
	}
	image_free(image);
	return NULL;
}

void
image_free(struct Image *image)
{
	if (image) {
		free(image->data);
		free(image);
	}
}
