#include "file_utils.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

#define ERR_IO "I/O error"
#define ERR_NO_FILE "file not found"
#define ERR_NO_MEM "out of memory"

size_t
file_read(const char *filename, char **r_buf, const char **r_err)
{
	assert(filename != NULL);
	assert(r_buf != NULL);

	size_t size = 0;
	const char *err = NULL;

	// open the file in binary read mode
	FILE *fp = fopen(filename, "rb");
	if (!fp) {
		err = ERR_NO_FILE;
		goto error;
	}

	// read file size
	fseek(fp, 0, SEEK_END);
	size = ftell(fp);
	rewind(fp);

	// allocate a buffer for its contents
	*r_buf = malloc(size + 1);
	if (!*r_buf) {
		err = ERR_NO_MEM;
		goto error;
	}

	(*r_buf)[size] = 0;  // NUL-terminator

	// read the file
	if (fread(*r_buf, 1, size, fp) != size) {
		err = ERR_IO;
		goto error;
	}

cleanup:
	if (fp) {
		fclose(fp);
	}
	return size;

error:
	size = 0;
	free(*r_buf);
	if (r_err) {
		*r_err = err;
	}
	goto cleanup;
}
