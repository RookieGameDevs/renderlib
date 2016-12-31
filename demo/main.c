#include <GL/glew.h>
#include <SDL.h>
#include <matlib.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

// renderlib headers
#include "renderer.h"
#include "mesh.h"

#define WIDTH 800
#define HEIGHT 600
#define UPDATE_INTERVAL 2.0f

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

	SDL_GL_SetSwapInterval(0);

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
update(float dt)
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

	struct MeshRenderProps props = {
		.cast_shadows = 1,
		.receive_shadows = 1,
		.projection = camera.projection
	};
	mat_ident(&props.model);
	mat_ident(&props.view);
	mat_translate(&props.view, 0, 0, -500);

	int ok = render_mesh(mesh, &props, NULL);

	renderer_present(NULL);
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

static void
update_stats(int fps, float render_time)
{
	render_time *= 1000.f; // in milliseconds
	const char *fmt = "FPS: %d, render time: %.2fms";
	int len = snprintf(NULL, 0, fmt, fps, render_time) + 1;
	char str[len + 1];
	str[len] = '\0'; // NUL-terminator
	snprintf(str, len, fmt, fps, render_time);
	SDL_SetWindowTitle(window, str);
}

static struct timespec
tstamp(void)
{
	struct timespec t;
	clock_gettime(CLOCK_MONOTONIC_RAW, &t);
	return t;
}

static float
timedelta(struct timespec a, struct timespec b)
{
	unsigned t1_secs = a.tv_sec & 0x1ffff;
	unsigned t2_secs = b.tv_sec & 0x1ffff;
	unsigned long long t1 = t1_secs * 1e9 + a.tv_nsec;
	unsigned long long t2 = t2_secs * 1e9 + b.tv_nsec;
	return (t1 - t2) / 1e9f;
}

int
main(int argc, char *argv[])
{
	int ok = (
		init(WIDTH, HEIGHT) &&
		load_resources()
	);

	int run = 1;

	struct timespec last_update = tstamp();
	float time_acc = 0, render_time = 0;
	int fps = 0;
	while (ok && run) {
		// compute time delta
		struct timespec now = tstamp();
		float dt = timedelta(now, last_update);
		last_update = now;

		// update stats
		time_acc += dt;
		if (time_acc >= UPDATE_INTERVAL) {
			time_acc -= UPDATE_INTERVAL;
			update_stats(fps / UPDATE_INTERVAL, render_time);
			fps = 0;
		} else {
			fps++;
		}

		// update the scene
		run &= update(dt);

		// render and measure rendering time
		now = tstamp();
		run &= render();
		render_time = timedelta(tstamp(), now);
	}

	cleanup_resources();
	shutdown();
	return ok ? EXIT_SUCCESS : EXIT_FAILURE;
}