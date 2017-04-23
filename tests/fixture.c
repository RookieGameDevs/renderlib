#include "fixture.h"
#include <SDL.h>
#include <check.h>
#include <renderlib.h>

static SDL_Window *window = NULL;
static SDL_GLContext *context = NULL;

void
setup(void)
{
	// initialize SDL video subsystem
	ck_assert_int_eq(SDL_Init(SDL_INIT_VIDEO), 0);

	// create window
	window = SDL_CreateWindow(
		"OpenGL",
		SDL_WINDOWPOS_CENTERED,
		SDL_WINDOWPOS_CENTERED,
		640,
		480,
		SDL_WINDOW_OPENGL
	);
	ck_assert(window != NULL);

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
	ck_assert(context != NULL);

	// initialize renderer
	ck_assert(renderer_init());
}

void
teardown(void)
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

void
render_frame(const char *testcase_name)
{
	SDL_SetWindowTitle(window, testcase_name);
	SDL_GL_SwapWindow(window);
	SDL_Event evt;
	while (SDL_PollEvent(&evt));
	SDL_Delay(1000);
}