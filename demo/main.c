#include "renderer.h"
#include <SDL.h>
#include <stdio.h>
#include <stdlib.h>

#define WIDTH 800
#define HEIGHT 600

static SDL_Window *window = NULL;
static SDL_GLContext *context = NULL;

static int
init(unsigned width, unsigned height)
{
	// initialize SDL video subsystem
	if (SDL_Init(SDL_INIT_VIDEO) != 0) {
		return 0;
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
		return 0;
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
		return 0;
	}

	return renderer_init(NULL);
}

static void
shutdown(void)
{
	// shutdown renderer
	renderer_shutdown();

	// destroy OpenGL context
	if (context) {
		SDL_GL_DeleteContext(context);
		context = NULL;
	}

	// destroy window
	if (window) {
		SDL_DestroyWindow(window);
		window = NULL;
	}

	// shutdown SDL
	SDL_Quit();
}

static int
update(void)
{
	SDL_Event evt;
	while (SDL_PollEvent(&evt)) {
		if (evt.type == SDL_QUIT ||
		    (evt.type == SDL_KEYDOWN && evt.key.keysym.sym == SDLK_ESCAPE)) {
			return 0;
		}
	}
	return 1;
}

static int
render(void)
{
	renderer_present();
	SDL_GL_SwapWindow(window);
	return 1;
}

int
main(int argc, char *argv[])
{
	int ok = init(WIDTH, HEIGHT);

	int run = 1;
	while (run) {
		run &= update();
		run &= render();
	}

	shutdown();
	return ok ? EXIT_SUCCESS : EXIT_FAILURE;
}