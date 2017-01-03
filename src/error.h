#pragma once

#include "string_utils.h"
#include <stdio.h>

enum {
	ERR_TRACEBACK_FULL = 1,
	ERR_GENERIC,
	ERR_NO_MEM,
	ERR_IO,
	ERR_NO_FILE,
	ERR_SDL,
	ERR_GLEW,
	ERR_OPENGL,
	ERR_LIBPNG,
	ERR_FREETYPE,
	ERR_INVALID_MESH,
	ERR_INVALID_IMAGE,
	ERR_UNSUPPORTED_IMAGE,
	ERR_UNSUPPORTED_IMAGE_FORMAT,
	ERR_TEXTURE_FORMAT,
	ERR_INVALID_FONT,
	ERR_INVALID_SHADER,
	ERR_SHADER_COMPILE,
	ERR_SHADER_LINK,
	ERR_SHADER_NO_UNIFORM,
	ERR_SHADER_NO_UNIFORM_BLOCK,
	ERR_SHADER_UNKNOWN_UNIFORM_TYPE,
	ERR_RENDER,
	ERR_RENDER_QUEUE_FULL,
};

#define err(code) error_push(code, NULL, __FILE__, __func__, __LINE__)
#define errf(code, ...) error_push(code, string_fmt(__VA_ARGS__), __FILE__, __func__, __LINE__)

void
error_push(int code, char *descr, const char *file, const char *context, int line);

void
error_dump_traceback(FILE *fp);

void
error_clear_traceback(void);
