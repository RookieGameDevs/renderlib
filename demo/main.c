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

enum {
	MOVE_LEFT = 1,
	MOVE_RIGHT = 1 << 1,
	MOVE_UP = 1 << 2,
	MOVE_DOWN = 1 << 3
};

static struct {
	int play_animation;
	int last_mouse_x;
	int last_mouse_y;
	int state;
	int move_flags;
} controls;

static struct Camera camera;
static struct Camera ui_camera;
static struct Light light;
static struct Scene *scene = NULL;
static struct Scene *ui_scene = NULL;

#define OBJECT_ROWS 10
#define OBJECT_COLUMNS 10
#define ROW_SPACING 0.5
#define COLUMN_SPACING 0.5
#define OBJECT_COUNT (OBJECT_ROWS * OBJECT_COLUMNS)

// main model
static struct Mesh *model_mesh = NULL;
static struct AnimationInstance *model_animation = NULL;
static struct Image *model_image = NULL;
static struct Texture *model_texture = NULL;
static struct Material model_material;
static struct MeshProps model_props;
static struct Object *objects[OBJECT_COUNT] = {NULL};

// terrain model
static struct Mesh *terrain_mesh = NULL;
static struct Image *terrain_image = NULL;
static struct Texture *terrain_texture = NULL;
static struct Material terrain_material;
static struct MeshProps terrain_props;
static struct Object *terrain_object = NULL;


#define PANEL_WIDTH (WIDTH / 4.0)
#define PANEL_HEIGHT 150

enum {
	IMAGE_WIDGET,
	TEXT_WIDGET
};

static struct Widget {
	int type;
	float size[2];
	float position[3];

	// initialized at load time
	struct Object *object;

	union {
		struct {
			const char *filename;
			float borders[4];

			// initialized at load time
			struct Image *image;
			struct Texture *texture;
			struct Quad *quad;
			struct QuadProps *props;
		} image;

		struct {
			const char *font_filename;
			int font_size;
			const char *label;
			float color[4];

			// initialized at load time
			struct Font *font;
			struct Text *text;
			struct TextProps *props;
		} text;
	};
} widgets[] = {
	// panel header
	{
		.type = IMAGE_WIDGET,
		.size = {PANEL_WIDTH, 48},
		.position = {0, 0, -0.3},
		.image.filename = "tests/data/blue_panel.png",
		.image.borders = {7, 5, 7, 5},
	},
	// panel frame
	{
		.type = IMAGE_WIDGET,
		.size = {PANEL_WIDTH, PANEL_HEIGHT},
		.position = {0, 40, -0.2},
		.image.filename = "tests/data/grey_panel.png",
		.image.borders = {7, 5, 7, 5},
	},
	// animation playback button - normal
	{
		.type = IMAGE_WIDGET,
		.size = {PANEL_WIDTH * 0.8, 48},
		.position = {(PANEL_WIDTH - PANEL_WIDTH * 0.8) / 2, 120, -0.1},
		.image.filename = "tests/data/button-normal.png",
		.image.borders = {6, 22, 6, 26},
	},
	// animation playback button - pressed
	{
		.type = IMAGE_WIDGET,
		.size = {PANEL_WIDTH * 0.8, 48},
		.position = {(PANEL_WIDTH - PANEL_WIDTH * 0.8) / 2, 120, -0.1},
		.image.filename = "tests/data/button-pressed.png",
		.image.borders = {6, 22, 6, 26},
	},
	// animation playback button - text label
	{
		.type = TEXT_WIDGET,
		.position = {40, 138, 0},
		.text.font_filename = "tests/data/kenvector_future.ttf",
		.text.font_size = 12,
		.text.label = "Play Animation",
		.text.color = {0.3, 0.3, 0.3, 1.0}
	},
	// title
	{
		.type = TEXT_WIDGET,
		.position = {20, 15, 0},
		.text.font_filename = "tests/data/kenvector_future.ttf",
		.text.font_size = 18,
		.text.label = "Controls",
		.text.color = {0.9, 0.9, 0.9, 1}
	},
	// FPS counter
	{
		.type = TEXT_WIDGET,
		.position = {12, 50, 0},
		.text.font_filename = "tests/data/kenvector_future.ttf",
		.text.font_size = 16,
		.text.label = "FPS: n/a",
		.text.color = {0.2, 0.2, 0.2, 1}
	},
	// frame time
	{
		.type = TEXT_WIDGET,
		.position = {12, 75, 0},
		.text.font_filename = "tests/data/kenvector_future.ttf",
		.text.font_size = 16,
		.text.label = "Frame: n/a",
		.text.color = {0.2, 0.2, 0.2, 1}
	}
};

#define WIDGET_COUNT (sizeof(widgets) / sizeof(struct Widget))

#define WIDGET_PLAYBACK_BUTTON_NORMAL (&widgets[2])
#define WIDGET_PLAYBACK_BUTTON_PRESSED (&widgets[3])
#define WIDGET_FPS_COUNTER (&widgets[6])
#define WIDGET_FRAME_TIME (&widgets[7])

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
	controls.move_flags = 0;

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
	Vec up = vec(0, 1, 0, 0);
	camera.position = eye;
	mat_lookatv(&camera.view, &eye, &origin, &up);

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
widget_cmp(const void *ptr1, const void *ptr2)
{
	int i = *(int*)ptr1, j = *(int*)ptr2;
	float z1 = widgets[i].position[2];
	float z2 = widgets[j].position[2];
	if (z1 > z2) {
		return -1;
	} else if (fabs(z1 - z2) < 1e-6) {
		return 0;
	}
	return 1;
}

static int
ui_handle_mouse(SDL_MouseButtonEvent *evt)
{
	// widgets indices sorted by related widgets' Z
	int indices[WIDGET_COUNT];
	for (unsigned i = 0; i < WIDGET_COUNT; i++) {
		indices[i] = i;
	}
	qsort(indices, WIDGET_COUNT, sizeof(int), widget_cmp);

	// pick a widget
	struct Widget *target = NULL;
	for (unsigned i = 0; i < WIDGET_COUNT; i++) {
		struct Widget *w = &widgets[indices[i]];
		float x0 = w->position[0];
		float y0 = w->position[1];
		float x1 = x0 + w->size[0];
		float y1 = y0 + w->size[1];
		if (evt->x >= x0 && evt->x <= x1 &&
		    evt->y >= y0 && evt->y <= y1) {
			target = w;
			break;
		}
	}

	// toggle animation play control
	if ((target == WIDGET_PLAYBACK_BUTTON_NORMAL ||
	     target == WIDGET_PLAYBACK_BUTTON_PRESSED) &&
	    evt->state == SDL_RELEASED) {
		controls.play_animation = !controls.play_animation;
	}

	return target != NULL;
}

static int
ui_update(void) {
	WIDGET_PLAYBACK_BUTTON_NORMAL->object->visible = !controls.play_animation;
	WIDGET_PLAYBACK_BUTTON_PRESSED->object->visible = controls.play_animation;
	return 1;
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
		}

		int bit = 0;
		if (evt.type == SDL_KEYDOWN || evt.type == SDL_KEYUP) {
			switch (evt.key.keysym.sym) {
			case SDLK_LEFT:
				bit = MOVE_LEFT;
				break;
			case SDLK_RIGHT:
				bit = MOVE_RIGHT;
				break;
			case SDLK_UP:
				bit = MOVE_UP;
				break;
			case SDLK_DOWN:
				bit = MOVE_DOWN;
				break;
			}

			if (evt.type == SDL_KEYDOWN) {
				controls.move_flags |= bit;
			} else {
				controls.move_flags &= ~bit;
			}
		}

		if (evt.type == SDL_MOUSEBUTTONDOWN) {
			if (!ui_handle_mouse(&evt.button)) {
				on_mouse_down(&evt.button);
			}
		} else if (evt.type == SDL_MOUSEBUTTONUP) {
			if (!ui_handle_mouse(&evt.button)) {
				on_mouse_up(&evt.button);
			}
		} else if (evt.type == SDL_MOUSEMOTION) {
			on_mouse_move(&evt.motion);
		}
	}

	// play animation
	if (controls.play_animation) {
		animation_instance_play(model_animation, dt);
	}

	float dx = 0, dy = 0;
	if (controls.move_flags & MOVE_LEFT) {
		dx--;
	}
	if (controls.move_flags & MOVE_RIGHT) {
		dx++;
	}
	if (controls.move_flags & MOVE_UP) {
		dy--;
	}
	if (controls.move_flags & MOVE_DOWN) {
		dy++;
	}

	dx *= dt;
	dy *= dt;
	Vec right = {{camera.view.data[0], camera.view.data[1], camera.view.data[2]}};
	vec_imulf(&right, dx);
	Vec forward = {{camera.view.data[8], camera.view.data[9], camera.view.data[10]}};
	vec_imulf(&forward, dy);
	vec_iadd(&camera.position, &right);
	vec_iadd(&camera.position, &forward);

	return 1;
}

static int
render(void)
{
	renderer_clear();

	int ok = (
		scene_render(scene, RENDER_TARGET_FRAMEBUFFER, &camera, &light) &&
		scene_render(ui_scene, RENDER_TARGET_OVERLAY, &ui_camera, NULL) &&
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
	    !(terrain_texture = texture_from_image(terrain_image, GL_TEXTURE_2D))) {
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

	return 1;
}

static int
load_image_widget(struct Widget *w)
{
	// load the widget image
	w->image.image = image_from_file(w->image.filename);
	if (!w->image.image) {
		return 0;
	}

	// create a rectangle texture from the loaded image
	w->image.texture = texture_from_image(w->image.image, GL_TEXTURE_RECTANGLE);
	if (!w->image.texture) {
		return 0;
	}

	// initialize the corresponding quad and props objects
	w->image.quad = malloc(sizeof(struct Quad));
	w->image.quad->width = w->size[0] > 0 ? w->size[0] : w->image.image->width;
	w->image.quad->height = w->size[1] > 0 ? w->size[1] : w->image.image->height;
	w->image.props = malloc(sizeof(struct QuadProps));
	w->image.props->texture = w->image.texture;
	w->image.props->color = vec(1, 1, 1, 1);
	w->image.props->opacity = 1.0;
	w->image.props->borders.left = w->image.borders[0];
	w->image.props->borders.top = w->image.borders[1];
	w->image.props->borders.right = w->image.borders[2];
	w->image.props->borders.bottom = w->image.borders[3];

	return 1;
}

static int
load_text_widget(struct Widget *w)
{
	// load the font
	w->text.font = font_from_file(w->text.font_filename, w->text.font_size);
	if (!w->text.font) {
		return 0;
	}

	// create text
	w->text.text = text_new(w->text.font);
	if (!w->text.text) {
		return 0;
	}
	text_set_string(w->text.text, w->text.label);

	// create props
	w->text.props = malloc(sizeof(struct TextProps));
	if (!w->text.props) {
		return 0;
	}
	w->text.props->color = vec(w->text.color[0], w->text.color[1], w->text.color[2], w->text.color[3]);
	w->text.props->opacity = 1.0;

	return 1;
}

static int
load_ui(void)
{
	int (*loaders[])(struct Widget *w) = {
		load_image_widget,
		load_text_widget
	};
	for (unsigned i = 0; i < WIDGET_COUNT; i++) {
		struct Widget *w = &widgets[i];
		if (!loaders[w->type](w)) {
			return 0;
		}
	}
	return 1;
}

static int
setup_ui(void)
{
	// create the scene
	if (!(ui_scene = scene_new())) {
		return 0;
	}

	for (unsigned i = 0; i < WIDGET_COUNT; i++) {
		struct Widget *w = &widgets[i];
		switch (w->type) {
		case IMAGE_WIDGET:
			w->object = scene_add_quad(ui_scene, w->image.quad, w->image.props);
			break;
		case TEXT_WIDGET:
			w->object = scene_add_text(ui_scene, w->text.text, w->text.props);
		}

		if (!w->object) {
			return 0;
		}

		// set widget position
		w->object->position = vec(
			w->position[0] - WIDTH / 2,
			HEIGHT / 2 - w->position[1],
			w->position[2],
			0
		);
	}
	return 1;
}

static int
setup_scene(void)
{
	if (!(scene = scene_new()) ||
	    !(terrain_object = scene_add_mesh(scene, terrain_mesh, &terrain_props))) {
		return 0;
	}

	for (size_t r = 0; r < OBJECT_ROWS; r++) {
		for (size_t c = 0; c < OBJECT_COLUMNS; c++) {
			struct Object *obj = scene_add_mesh(
				scene,
				model_mesh,
				&model_props
			);
			if (!obj) {
				return 0;
			}

			obj->position = vec(
				(c % 2 ? -1 : 1) * c * COLUMN_SPACING,
				0,
				(r % 2 ? -1 : 1) * r * ROW_SPACING,
				1
			);
			objects[r * OBJECT_ROWS + c] = obj;
		}
	}

	terrain_object->scale = vec(2, 2, 2, 0);

	return 1;
}

static void
cleanup_resources(void)
{
	for (unsigned i = 0; i < WIDGET_COUNT; i++) {
		struct Widget *w = &widgets[i];
		switch (w->type) {
		case IMAGE_WIDGET:
			free(w->image.props);
			free(w->image.quad);
			texture_free(w->image.texture);
			image_free(w->image.image);
			break;
		case TEXT_WIDGET:
			free(w->text.props);
			text_free(w->text.text);
			font_free(w->text.font);
		}
	}
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
	{
		const char *fmt = "FPS: %d";
		int len = snprintf(NULL, 0, fmt, fps) + 1;
		char str[len + 1];
		str[len] = '\0'; // NUL-terminator
		snprintf(str, len, fmt, fps);
		text_set_string(WIDGET_FPS_COUNTER->text.text, str);
	}
	{
		render_time *= 1000.f; // in milliseconds
		const char *fmt = "Frame: %.2fms";
		int len = snprintf(NULL, 0, fmt, render_time) + 1;
		char str[len + 1];
		str[len] = '\0'; // NUL-terminator
		snprintf(str, len, fmt, render_time);
		text_set_string(WIDGET_FRAME_TIME->text.text, str);
	}
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
		load_ui() &&
		setup_scene() &&
		setup_ui()
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
		run &= ui_update();
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
