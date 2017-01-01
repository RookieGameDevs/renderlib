#include "error.h"
#include <stdlib.h>
#include <string.h>

#define ERROR_TRACEBACK_SIZE 100

static struct Error {
	int code;
	char *descr;
	const char *file;
	const char *context;
	int line;
} traceback[ERROR_TRACEBACK_SIZE];

static size_t traceback_len = 0;
static int initialized = 0;

static const char *error_names[] = {
	// ERR_TRACEBACK_FULL
	"full error traceback",
	// ERR_GENERIC
	"generic error",
	// ERR_NO_MEM
	"out of memory",
	// ERR_IO
	"I/O error",
	// ERR_NO_FILE
	"file not found",
	// ERR_SDL
	"SDL internal error",
	// ERR_GLEW
	"GLEW internal error",
	// ERR_OPENGL
	"OpenGL internal error",
	// ERR_LIBPNG
	"libpng internal error",
	// ERR_FREETYPE
	"freetype2 internal error",
	// ERR_INVALID_MESH
	"invalid mesh",
	// ERR_INVALID_IMAGE
	"invalid image",
	// ERR_UNSUPPORTED_IMAGE
	"unsupported image type",
	// ERR_UNSUPPORTED_IMAGE_FORMAT
	"unsupported image format",
	// ERR_TEXTURE_FORMAT
	"bad texture format",
	// ERR_INVALID_FONT
	"invalid font",
	// ERR_INVALID_SHADER
	"invalid shader",
	// ERR_SHADER_COMPILE
	"shader compile error",
	// ERR_SHADER_LINK
	"shader link error",
	// ERR_SHADER_NO_UNIFORM
	"shader uniform not found",
	// ERR_SHADER_NO_UNIFORM_BLOCK
	"shader uniform block not found",
	// ERR_SHADER_UNKNOWN_UNIFORM_TYPE
	"shader uniform type unknown",
	// ERR_RENDER
	"render error",
	// ERR_RENDER_QUEUE_FULL
	"full render queue"
};

void
error_push(int code, char *descr, const char *file, const char *context, int line)
{
	if (!initialized) {
		memset(traceback, 0, sizeof(traceback));
		atexit(error_clear_traceback);
		initialized = 1;
	}

	if (traceback_len == ERROR_TRACEBACK_SIZE - 1) {
		int line = __LINE__ - 1;
		struct Error error = {
			.code = ERR_TRACEBACK_FULL,
			.descr = NULL,
			.file = __FILE__,
			.context = __func__,
			.line = line
		};
		traceback[traceback_len++] = error;
	} else if (traceback_len < ERROR_TRACEBACK_SIZE - 1) {
		// attempt to strip the path and use file base name only
		const char *basename = strrchr(file, '/');
		if (basename) {
			file = basename + 1;
		}
		struct Error error = {
			.code = code,
			.descr = descr,
			.file = file,
			.context = context,
			.line = line
		};
		traceback[traceback_len++] = error;
	}
}

void
error_dump_traceback(FILE *fp)
{
	if (!initialized) {
		return;
	}

	for (size_t i = 0; i < traceback_len; i++) {
		struct Error *error = &traceback[i];
		const char *fmt = (
			error->descr
			? "%s:%d %s\n\t%s: %s\n"
			: "%s:%d %s\n\t%s\n"
		);
		fprintf(
			fp,
			fmt,
			error->file,
			error->line,
			error->context,
			error_names[error->code - 1],
			error->descr
		);
	}
}

void
error_clear_traceback(void)
{
	if (!initialized) {
		return;
	}

	for (size_t i = 0; i < traceback_len; i++) {
		free(traceback[i].descr);
	}
	memset(traceback, 0, sizeof(traceback));
	traceback_len = 0;
}