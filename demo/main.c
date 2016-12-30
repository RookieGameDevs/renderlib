#include <GL/glew.h>
#include <SDL.h>
#include <matlib.h>
#include <stdio.h>
#include <stdlib.h>

// renderlib headers
#include "renderer.h"
#include "mesh.h"
#include "draw.h"

#define WIDTH 800
#define HEIGHT 600

static SDL_Window *window = NULL;
static SDL_GLContext *context = NULL;

static struct {
	Mat projection;
} camera;

static struct Mesh *mesh = NULL;

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

	// one-off OpenGL initializations
	glClearColor(0.3, 0.3, 0.3, 1.0);
	glEnable(GL_DEPTH_TEST);

	// initialize camera
	mat_ident(&camera.projection);
	mat_persp(
		&camera.projection,
		60.0f,
		WIDTH / (float)HEIGHT,
		100,
		1000
	);

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
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	Mat model, view;
	mat_ident(&model);
	mat_ident(&view);
	mat_translate(&view, 0, 0, -500);

	int ok = draw_mesh(
		mesh,
		&model,
		&view,
		&camera.projection
	);

	renderer_present();
	SDL_GL_SwapWindow(window);
	return ok;
}

static int
load_resources(void)
{
	if (!(mesh = mesh_from_file("tests/data/zombie.mesh", NULL))) {
		return 0;
	}
	return 1;
}

static void
cleanup_resources(void)
{
	mesh_free(mesh);
}

int
main(int argc, char *argv[])
{
	int ok = (
		init(WIDTH, HEIGHT) &&
		load_resources()
	);

	int run = 1;
	while (ok && run) {
		run &= update();
		run &= render();
	}

	cleanup_resources();
	shutdown();
	return ok ? EXIT_SUCCESS : EXIT_FAILURE;
}