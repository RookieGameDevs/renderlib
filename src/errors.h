#pragma once

typedef enum err_t {
	ERR_NO_MEM = 1,
	ERR_IO,
	ERR_NO_FILE,
	ERR_SDL,
	ERR_GLEW,
	ERR_OPENGL,
	ERR_LIBPNG,
	ERR_INVALID_MESH,
	ERR_INVALID_IMAGE,
} err_t;
