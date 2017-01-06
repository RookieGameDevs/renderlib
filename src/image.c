#include "error.h"
#include "file_utils.h"
#include "image.h"
#include <assert.h>
#include <jerror.h>
#include <jpeglib.h>
#include <png.h>
#include <setjmp.h>
#include <stdlib.h>
#include <string.h>

struct Buffer {
	size_t size;
	size_t offset;
	void *data;
};

static size_t
buffer_read(struct Buffer *buf, void *dst, size_t len)
{
	if (buf->offset + len > buf->size) {
		return 0;
	}
	memcpy(dst, buf->data + buf->offset, len);
	buf->offset += len;
	return len;
}

static void
user_png_read_func(png_structp png_ptr, png_bytep dst, png_size_t len)
{
	struct Buffer *buf = png_get_io_ptr(png_ptr);
	if (!buffer_read(buf, dst, len)) {
		png_error(png_ptr, "end of buffer");
	}
}

static void*
read_png(struct Buffer *buf, unsigned *r_width, unsigned *r_height, int *fmt) {
	void *data = NULL;
	png_structp png_ptr = NULL;
	png_infop info_ptr = NULL;
	png_bytepp rows = NULL;

	// attempt to read 8 bytes and check whether we're reading a PNG file
	size_t hdr_size = 8;
	unsigned char hdr[hdr_size];
	if (buffer_read(buf, hdr, hdr_size) != hdr_size ||
	    png_sig_cmp(hdr, 0, hdr_size) != 0) {
		err(ERR_INVALID_IMAGE);
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
		err(ERR_LIBPNG);
		goto error;
	}

	info_ptr = png_create_info_struct(png_ptr);
	if (!info_ptr) {
		err(ERR_LIBPNG);
		goto error;
	}

	// set the error handling longjmp point
	if (setjmp(png_jmpbuf(png_ptr))) {
		err(ERR_INVALID_IMAGE);
		goto error;
	}

	// init reading
	png_set_read_fn(png_ptr, buf, user_png_read_func);
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

	// set format
	if (color_type == PNG_COLOR_TYPE_RGB) {
		*fmt = IMAGE_FORMAT_RGB;
	} else if (color_type == PNG_COLOR_TYPE_RGBA) {
		*fmt = IMAGE_FORMAT_RGBA;
	} else {
		err(ERR_UNSUPPORTED_IMAGE_FORMAT);
		goto error;
	}

	// allocate space for image data
	size_t rowbytes = png_get_rowbytes(png_ptr, info_ptr);
	data = malloc(height * rowbytes);
	if (!data) {
		err(ERR_NO_MEM);
		goto error;
	}

	// setup an array of image row pointers
	rows = malloc(height * sizeof(png_bytep));
	if (!rows) {
		err(ERR_NO_MEM);
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
	return data;

error:
	free(data);
	data = NULL;
	goto cleanup;
}

static void*
read_jpeg(struct Buffer *buf, unsigned *width, unsigned *height, int *fmt)
{
	struct jpeg_decompress_struct cinfo;
	struct jpeg_error_mgr jerr;
	void *data = NULL;

	// initialize JPEG error manager and decompressor
	jpeg_std_error(&jerr);
	cinfo.err = &jerr;
	jpeg_create_decompress(&cinfo);

	// feed the buffer to decompressor
	jpeg_mem_src(&cinfo, buf->data, buf->size);

	// read the header
	if (jpeg_read_header(&cinfo, 1) != JPEG_HEADER_OK) {
		err(ERR_INVALID_IMAGE);
		goto error;
	}

	// decompress always into color RGB format, even if incoming data is in
	// grayscale or other formats
	cinfo.out_color_space = JCS_RGB;

	// decompress the image
	jpeg_start_decompress(&cinfo);

	// allocate buffer for image data
	size_t row_size = cinfo.output_width * cinfo.output_components;
	data = malloc(row_size * cinfo.output_height);
	if (!data) {
		err(ERR_NO_MEM);
		goto error;
	}

	// read the image one row at a time
	while (cinfo.output_scanline < cinfo.output_height) {
		// jpeg_read_scanlines() accepts an array of pointers to
		// scanline starts, in this case we have an array of just one
		// pointer
		JSAMPROW scanlines = { data + row_size * cinfo.output_scanline };
		jpeg_read_scanlines(
			&cinfo,
			&scanlines,
			1
		);
	}

	// retrieve size and set format
	*width = cinfo.output_width;
	*height = cinfo.output_height;
	*fmt = IMAGE_FORMAT_RGB;

cleanup:
	// finish decompress phase
	jpeg_finish_decompress(&cinfo);

	// destroy decompressor
	jpeg_destroy_decompress(&cinfo);

	return data;

error:
	free(data);
	data = NULL;
	goto cleanup;
}

struct Image*
image_from_buffer(const void *data, size_t size, int codec)
{
	assert(data != NULL);
	assert(size > 0);

	struct Image *image = NULL;

	// pick proper reader
	void* (*reader)(struct Buffer*, unsigned*, unsigned*, int*) = NULL;
	switch (codec) {
	case IMAGE_CODEC_PNG:
		reader = read_png;
		break;
	case IMAGE_CODEC_JPEG:
		reader = read_jpeg;
		break;
	}
	if (!reader) {
		err(ERR_UNSUPPORTED_IMAGE);
		return NULL;
	}

	// allocate image struct
	if (!(image = malloc(sizeof(struct Image)))) {
		err(ERR_NO_MEM);
		return NULL;
	}

	// read image
	struct Buffer buffer = {
		.data = (void*)data,
		.size = size,
		.offset = 0
	};
	image->data = reader(
		&buffer,
		&image->width,
		&image->height,
		&image->format
	);
	if (!image->data) {
		err(ERR_INVALID_IMAGE);
		image_free(image);
		return NULL;
	}

	return image;
}

struct Image*
image_from_file(const char *filename)
{
	assert(filename != NULL);

	// pick image reader function based on file extension
	const char *ext = strrchr(filename, '.');
	int codec = 0;
	if (strncmp(ext, ".png", 3) == 0) {
		codec = IMAGE_CODEC_PNG;
	} else if (strncmp(ext, ".jpg", 3) == 0 ||
	           strncmp(ext, ".jpeg", 4) == 0) {
		codec = IMAGE_CODEC_JPEG;
	}

	// read file contents
	char *data = NULL;
	size_t size = file_read(filename, &data);
	if (!size) {
		return NULL;
	}

	// create image from just read file contents
	struct Image *image = image_from_buffer(data, size, codec);

	free(data);

	return image;
}

void
image_free(struct Image *image)
{
	if (image) {
		free(image->data);
		free(image);
	}
}
