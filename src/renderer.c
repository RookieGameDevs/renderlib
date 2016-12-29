#include "renderer.h"
#include <GL/glew.h>
#include <SDL.h>
#include <assert.h>
#include <stdlib.h>

static SDL_Window *window = NULL;
static SDL_GLContext *context = NULL;
static int initialized = 0;
static int registered_at_exit = 0;

#define ERR_SDL_INIT "SDL initialization error"
#define ERR_OPENGL_INIT "OpenGL context creation error"
#define ERR_GLEW_INIT "GLEW initialization error"

int
renderer_init(unsigned width, unsigned height, const char **r_err)
{
	const char *err = NULL;

	if (initialized) {
		renderer_shutdown();
	}

	// initialize SDL video subsystem
	if (!SDL_WasInit(SDL_INIT_VIDEO) && SDL_Init(SDL_INIT_VIDEO) != 0) {
		err = ERR_SDL_INIT;
		goto error;
	}

	// create window
	window = SDL_CreateWindow(
		"OpenGL",
		SDL_WINDOWPOS_CENTERED,
		SDL_WINDOWPOS_CENTERED,
		width,
		height,
		SDL_WINDOW_OPENGL
	);
	if (!window) {
		err = ERR_SDL_INIT;
		goto error;
	}

	// initialize OpenGL context
	SDL_GL_SetAttribute(
		SDL_GL_CONTEXT_PROFILE_MASK,
		SDL_GL_CONTEXT_PROFILE_CORE
	);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
	SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
	SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);

	context = SDL_GL_CreateContext(window);
	if (!context) {
		err = ERR_OPENGL_INIT;
		goto error;
	}

	// initialize GLEW
	glewExperimental = GL_TRUE;
	if (glewInit() != 0) {
		err = ERR_GLEW_INIT;
		goto error;
	}
	glGetError(); // silence any errors produced during GLEW initialization

	initialized = 1;
	if (!registered_at_exit) {
		atexit(renderer_shutdown);
		registered_at_exit = 1;
	}
	return 1;

error:
	renderer_shutdown();
	if (r_err) {
		*r_err = err;
	}
	return 0;
}

int
renderer_present(void)
{
	assert(initialized);

	SDL_GL_SwapWindow(window);

	return 1;
}

void
renderer_shutdown(void)
{
	if (context) {
		SDL_GL_DeleteContext(context);
		context = NULL;
	}
	if (window) {
		SDL_DestroyWindow(window);
		window = NULL;
	}

	SDL_Quit();
	initialized = 0;
}
