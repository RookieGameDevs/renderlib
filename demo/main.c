#ifndef __APPLE__
# define _POSIX_C_SOURCE 199309L
#endif

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

enum {
	STATE_INITIAL,
	STATE_ROTATE_CAMERA
};

static struct {
	int play_animation;
	int last_mouse_x;
	int last_mouse_y;
	int state;
} controls;

static struct Camera camera;
static struct Camera ui_camera;
static struct Light light;
static struct Scene *scene = NULL;
static struct Scene *ui_scene = NULL;

// main model
static struct Mesh *model_mesh = NULL;
static struct AnimationInstance *model_animation = NULL;
static struct Image *model_image = NULL;
static struct Texture *model_texture = NULL;
static struct Material model_material;
static struct MeshProps model_props;
static struct Object *model_object = NULL;

// terrain model
static struct Mesh *terrain_mesh = NULL;
static struct Image *terrain_image = NULL;
static struct Texture *terrain_texture = NULL;
static struct Material terrain_material;
static struct MeshProps terrain_props;
static struct Object *terrain_object = NULL;

// fps counter
static struct Text *fps_text = NULL;
static struct Font *fps_font = NULL;
static struct TextProps fps_props;
static struct Object *fps_object = NULL;

// close button
static struct Quad btn_quad = { .width = 38, .height = 36 };
static struct Image *btn_image = NULL;
static struct Texture *btn_texture = NULL;
static struct QuadProps btn_props;
static struct Object *btn_object = NULL;

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

	// initialize controls
	controls.play_animation = 0;
	controls.state = STATE_INITIAL;

	float aspect = WIDTH / (float)HEIGHT;

	// initialize 2D orthographic camera for UI
	camera_init_orthographic(
		&ui_camera,
		-WIDTH / 2,
		+WIDTH / 2,
		+HEIGHT / 2,
		-HEIGHT / 2,
		0,
		1
	);

	// initialize camera
	camera_init_perspective(
		&camera,
		30.0f,
		aspect,
		1,
		50
	);
	Vec eye = vec(5, 5, 5, 0);
	Vec origin = vec(0, 0, 0, 0);
	camera_set_position(&camera, &eye);
	camera_look_at(&camera, &origin);

	// initialize light
	light.color = vec(1, 1, 1, 1);
	light.ambient_intensity = 0.3;
	light.diffuse_intensity = 1.0;
	light.direction = vec(0, -5, -5, 0);
	vec_norm(&light.direction);

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

static void
on_mouse_down(struct SDL_MouseButtonEvent *evt)
{
	switch (controls.state) {
	case STATE_INITIAL:
		if (evt->button == SDL_BUTTON_LEFT) {
			controls.state = STATE_ROTATE_CAMERA;
			controls.last_mouse_x = evt->x;
			controls.last_mouse_y = evt->y;
		}
		break;
	}
}

static void
on_mouse_up(struct SDL_MouseButtonEvent *evt)
{
	switch (controls.state) {
	case STATE_ROTATE_CAMERA:
		if (evt->button == SDL_BUTTON_LEFT) {
			controls.state = STATE_INITIAL;
		}
		break;
	}
}

static Vec
get_arcball_vector(float x, float y)
{
	int w, h;
	SDL_GetWindowSize(window, &w, &h);

	Vec v = vec(
		x / w * 2.0 - 1.0,
		-(y / h * 2.0 - 1.0),
		0,
		0
	);

	float sq_dist = v.data[0] * v.data[0] + v.data[1] * v.data[1];
	if (sq_dist <= 1.0) {
		v.data[2] = sqrt(1 - sq_dist);
	} else {
		vec_norm(&v);
	}

	return v;
}

static void
rotate_view(int x, int y)
{
	// compute vectors from arcball surface to origin
	Vec v2 = get_arcball_vector(x, y);
	Vec v1 = get_arcball_vector(controls.last_mouse_x, controls.last_mouse_y);

	// compute the angle between them
	float angle = acos(fmin(1.0, vec_dot(&v1, &v2)));

	// determine the rotation axis
	Vec axis;
	vec_cross(&v1, &v2, &axis);
	if (vec_mag(&axis) < 1e-3f) {
		return;
	}
	vec_norm(&axis);

	Vec cam_axis;
	Mat inv_view_m;
	mat_inverse(&camera.view, &inv_view_m);
	mat_mulv(&inv_view_m, &axis, &cam_axis);
	axis = cam_axis;

	Mat rot_m;
	mat_ident(&rot_m);
	mat_rotatev(&rot_m, &axis, angle);
	Mat tmp;
	mat_mul(&camera.view, &rot_m, &tmp);
	camera.view = tmp;

	controls.last_mouse_x = x;
	controls.last_mouse_y = y;
}

static void
on_mouse_move(struct SDL_MouseMotionEvent *evt)
{
	switch (controls.state) {
	case STATE_ROTATE_CAMERA:
		rotate_view(evt->x, evt->y);
		break;
	}
}

static int
update(float dt)
{
	// handle events
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
		} else if (evt.type == SDL_MOUSEBUTTONDOWN &&
			   evt.button.x >= WIDTH - 40 &&
			   evt.button.x <= WIDTH &&
			   evt.button.y >= 2 &&
		           evt.button.y <= 40) {
			return 0;
		}

		if (evt.type == SDL_MOUSEBUTTONDOWN) {
			on_mouse_down(&evt.button);
		} else if (evt.type == SDL_MOUSEBUTTONUP) {
			on_mouse_up(&evt.button);
		} else if (evt.type == SDL_MOUSEMOTION) {
			on_mouse_move(&evt.motion);
		}
	}

	// play animation
	if (controls.play_animation) {
		animation_instance_play(model_animation, dt);
	}

	return 1;
}

static int
render(void)
{
	renderer_clear();

	int ok = (
		scene_render(scene, &camera, &light) &&
		scene_render(ui_scene, &ui_camera, NULL) &&
		renderer_present()
	);

	SDL_GL_SwapWindow(window);

	return ok;
}

static int
load_resources(void)
{
	if (!(model_mesh = mesh_from_file("tests/data/zombie.mesh")) ||
	    !(model_animation = animation_instance_new(&model_mesh->animations[0])) ||
	    !(model_image = image_from_file("tests/data/zombie.jpg")) ||
	    !(model_texture = texture_from_image(model_image, GL_TEXTURE_2D)) ||
	    !(terrain_mesh = mesh_from_file("tests/data/plane.mesh")) ||
	    !(terrain_image = image_from_file("tests/data/grass.jpg")) ||
	    !(terrain_texture = texture_from_image(terrain_image, GL_TEXTURE_2D)) ||
	    !(fps_font = font_from_file("tests/data/courier.ttf", 16)) ||
	    !(fps_text = text_new(fps_font)) ||
	    !(btn_image = image_from_file("tests/data/close_btn.png")) ||
	    !(btn_texture = texture_from_image(btn_image, GL_TEXTURE_RECTANGLE))) {
		errf(ERR_GENERIC, "failed to load resources", 0);
		return 0;
	}

	// model material
	model_material.texture = model_texture;
	model_material.receive_light = 1;
	model_material.specular_intensity = 0.3;
	model_material.specular_power = 4;

	// model mesh props
	memset(&model_props, 0, sizeof(struct MeshProps));
	model_props.cast_shadows = 1;
	model_props.receive_shadows = 1;
	model_props.animation = model_animation;
	model_props.material = &model_material;

	// terrain material
	terrain_material.texture = terrain_texture;
	terrain_material.receive_light = 1;

	// terrain mesh props
	memset(&terrain_props, 0, sizeof(struct MeshProps));
	terrain_props.cast_shadows = 0;
	terrain_props.receive_shadows = 1;
	terrain_props.material = &terrain_material;

	// FPS counter text props
	memset(&fps_props, 0, sizeof(struct TextProps));
	fps_props.color = vec(0.5, 1.0, 0.5, 1.0);
	fps_props.opacity = 1.0;

	// close button quad props
	memset(&btn_props, 0, sizeof(struct QuadProps));
	btn_props.color = vec(1, 1, 1, 1);
	btn_props.opacity = 1.0;
	btn_props.texture = btn_texture;

	return 1;
}

static int
setup_scene(void)
{
	if (!(scene = scene_new()) ||
	    !(ui_scene = scene_new()) ||
	    !(model_object = scene_add_mesh(scene, model_mesh, &model_props)) ||
	    !(terrain_object = scene_add_mesh(scene, terrain_mesh, &terrain_props)) ||
	    !(fps_object = scene_add_text(ui_scene, fps_text, &fps_props)) ||
	    !(btn_object = scene_add_quad(ui_scene, &btn_quad, &btn_props))) {
		return 0;
	}

	terrain_object->scale = vec(2, 2, 2, 0);
	fps_object->position = vec(-WIDTH / 2 + 10, HEIGHT / 2 - 10, 0, 0);
	btn_object->position = vec(WIDTH / 2 - 40, HEIGHT / 2 - 2, 0, 0);

	return 1;
}

static void
cleanup_resources(void)
{
	texture_free(btn_texture);
	image_free(btn_image);
	text_free(fps_text);
	font_free(fps_font);
	texture_free(terrain_texture);
	image_free(terrain_image);
	mesh_free(terrain_mesh);
	texture_free(model_texture);
	image_free(model_image);
	animation_instance_free(model_animation);
	mesh_free(model_mesh);
	scene_free(ui_scene);
	scene_free(scene);
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
		load_resources() &&
		setup_scene()
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
