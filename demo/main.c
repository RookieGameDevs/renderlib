#define _POSIX_C_SOURCE 199309L

#include <GL/glew.h>
#include <SDL.h>
#include <matlib.h>
#include <renderlib.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define WIDTH 800
#define HEIGHT 600
#define UPDATE_INTERVAL 2.0f

static SDL_Window *window = NULL;
static SDL_GLContext *context = NULL;

static Mat ui_projection;

static struct {
	Vec eye;
	Mat view;
	Mat projection;
} camera;

static struct Light light;

static struct {
	int play_animation;
} controls;

// main model
static struct Mesh *mesh = NULL;
static struct AnimationInstance *animation = NULL;
static struct Image *image = NULL;
static struct Texture *texture = NULL;
static struct Material material;

// terrain model
static struct Mesh *terrain_mesh = NULL;
static struct Image *grass_img = NULL;
static struct Texture *terrain_texture = NULL;
static struct Material terrain_material;

// fps counter
static struct Text *fps_text = NULL;
static struct Font *font = NULL;

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

	// disable vsync
	SDL_GL_SetSwapInterval(0);

	float aspect = WIDTH / (float)HEIGHT;

	// initialize 2D projection matrix
	mat_ident(&ui_projection);
	mat_ortho(
		&ui_projection,
		-WIDTH / 2,
		+WIDTH / 2,
		+HEIGHT / 2,
		-HEIGHT / 2,
		0,
		1
	);

	// initialize camera
	camera.eye = vec(5, 5, 5, 0);
	mat_ident(&camera.projection);
	mat_persp(
		&camera.projection,
		30.0f,
		WIDTH / (float)HEIGHT,
		1,
		100
	);
	mat_lookat(
		&camera.view,
		5, 5, 5, // eye
		0, 0, 0, // target
		0, 1, 0  // up
	);

	// initialize light
	light.direction = vec(0, -5, -5, 0);
	vec_norm(&light.direction);
	light.color = vec(1, 1, 1, 1);
	light.ambient_intensity = 0.3;
	light.diffuse_intensity = 1.0;
	Mat view, proj;
	mat_ortho(
		&proj,
		-5.0,
		+5.0,
		+5.0 * aspect,
		-5.0 * aspect,
		0,
		10
	);
	mat_lookat(
		&view,
		0.0, 5.0, 5.0, // eye
		0.0, 0.0, 0.0, // target
		0.0, 1.0, 0.0  // up
	);
	mat_mul(&proj, &view, &light.transform);

	// initialize controls
	controls.play_animation = 0;

	return renderer_init();
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
		if (evt.type == SDL_QUIT) {
			return 0;
		} else if (evt.type == SDL_KEYUP) {
			switch (evt.key.keysym.sym) {
			case SDLK_ESCAPE:
				return 0;
			case SDLK_SPACE:
				controls.play_animation = !controls.play_animation;
				break;
			}
		}
	}

	// play animation
	if (controls.play_animation) {
		animation_instance_play(animation, dt);
	}

	return 1;
}

static int
render(void)
{
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	Mat identity;
	mat_ident(&identity);

	struct MeshRenderProps mesh_props = {
		.eye = camera.eye,
		.model = mesh->transform,
		.view = camera.view,
		.projection = camera.projection,
		.cast_shadows = 1,
		.receive_shadows = 1,
		.light = &light,
		.animation = animation,
		.material = &material
	};

	struct MeshRenderProps terrain_props = {
		.eye = camera.eye,
		.model = terrain_mesh->transform,
		.view = camera.view,
		.projection = camera.projection,
		.cast_shadows = 0,
		.receive_shadows = 1,
		.light = &light,
		.animation = NULL,
		.material = &terrain_material
	};
	mat_scale(&terrain_props.model, 2, 2, 1);

	struct TextRenderProps text_props = {
		.model = identity,
		.view = identity,
		.projection = ui_projection,
		.color = vec(0.5, 1.0, 0.5, 1.0),
		.opacity = 1.0
	};
	mat_translate(&text_props.model, -WIDTH / 2 + 10, HEIGHT / 2 - 10, 0);

	int ok = (
		render_mesh(mesh, &mesh_props) &&
		render_mesh(terrain_mesh, &terrain_props) &&
		render_text(fps_text, &text_props) &&
		renderer_present()
	);
	SDL_GL_SwapWindow(window);
	return ok;
}

static int
load_resources(void)
{
	if (!(mesh = mesh_from_file("tests/data/zombie.mesh")) ||
	    !(animation = animation_instance_new(&mesh->animations[0])) ||
	    !(image = image_from_file("tests/data/zombie.jpg")) ||
	    !(texture = texture_from_image(image, GL_TEXTURE_2D)) ||
	    !(terrain_mesh = mesh_from_file("tests/data/plane.mesh")) ||
	    !(grass_img = image_from_file("tests/data/grass.jpg")) ||
	    !(terrain_texture = texture_from_image(grass_img, GL_TEXTURE_2D)) ||
	    !(font = font_from_file("tests/data/courier.ttf", 16)) ||
	    !(fps_text = text_new(font))) {
		errf(ERR_GENERIC, "failed to load resources", 0);
		return 0;
	}

	material.texture = texture;
	material.receive_light = 1;
	material.specular_intensity = 0.3;
	material.specular_power = 4;
	terrain_material.texture = terrain_texture;
	terrain_material.receive_light = 1;

	return 1;
}

static void
cleanup_resources(void)
{
	text_free(fps_text);
	font_free(font);
	texture_free(terrain_texture);
	image_free(grass_img);
	mesh_free(terrain_mesh);
	texture_free(texture);
	image_free(image);
	animation_instance_free(animation);
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
	text_set_string(fps_text, str);
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
		ok &= render();
		render_time = timedelta(tstamp(), now);
	}

	cleanup_resources();
	shutdown();

	if (!ok) {
		error_dump_traceback(stderr);
		return EXIT_FAILURE;
	}
	return EXIT_SUCCESS;
}
