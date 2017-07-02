// renderlib
#include "renderlib.h"
// standard C library
#include <assert.h>
#include <stdlib.h>
#include <string.h>

// defined in shadow_pass.c
extern struct RenderPassCls shadow_pass_cls;

// defined in text_pass.c
extern struct RenderPassCls text_pass_cls;

// defined in quad_pass.c
extern struct RenderPassCls quad_pass_cls;

// defined in phong_pass.c
extern struct RenderPassCls phong_pass_cls;

// render pass factory classes
static const struct RenderPassCls *pass_classes[] = {
	&shadow_pass_cls,
	&text_pass_cls,
	&quad_pass_cls,
	&phong_pass_cls,
};

// computed size of currently active render passes
#define RENDER_PASS_COUNT (sizeof(pass_classes) / sizeof(struct RenderPassCls*))

// render pass class instances
static struct RenderPass *passes[RENDER_PASS_COUNT] = { NULL };

// maximum size of the render queue
#define RENDER_QUEUE_SIZE 1000

// render queue
static struct RenderQueue {
	struct RenderCommand commands[RENDER_QUEUE_SIZE];
	size_t len;
} queue = { .len = 0 };

// maximum number of pass shader uniforms
#define MAX_UNIFORM_COUNT 1024

// cache of current render pass shader uniform values
static struct ShaderUniformValue current_pass_values[MAX_UNIFORM_COUNT];

static int
render_command_cmp(const void *a_ptr, const void *b_ptr)
{
	const struct RenderCommand *a = a_ptr, *b = b_ptr;
	int pass_diff = a->pass - b->pass;
	if (pass_diff != 0) {
		return pass_diff;
	}
	return a->geometry - b->geometry;
}

int
renderer_init(void)
{
	// initialize GLEW
	glewExperimental = GL_TRUE;
	static int glew_initialized = 0;
	if (!glew_initialized && glewInit() != GLEW_OK) {
		err(ERR_GLEW);
		return 0;
	} else {
		glew_initialized = 1;
	}
	// silence any errors produced during GLEW initialization
	glGetError();

	// one-off OpenGL initializations
	glClearColor(0.3, 0.3, 0.3, 1.0);

	// initialize render passes
	for (unsigned i = 0; i < RENDER_PASS_COUNT; i++) {
		if (!(passes[i] = pass_classes[i]->alloc())) {
			errf(
				ERR_GENERIC,
				"%s pass initialization failed",
				pass_classes[i]->name
			);
			goto error;
		}
	}

	return 1;

error:
	renderer_shutdown();
	return 0;
}

void
renderer_clear(void)
{
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

int
renderer_present(void)
{
	// sort the render queue
	qsort(
		queue.commands,
		queue.len,
		sizeof(struct RenderCommand),
		render_command_cmp
	);

	// perform actual draw
	int current_pass = -1;
	struct Shader *current_shader = NULL;
	struct Geometry *current_geom = NULL;
	for (size_t i = 0; i < queue.len; i++) {
		struct RenderCommand *cmd = &queue.commands[i];

		// switch to next render pass, if needed
		if (current_pass != cmd->pass) {
			// exit current pass
			if (current_pass != -1) {
				if (!passes[current_pass]->cls->exit(passes[current_pass])) {
					// TODO: push proper error to stack
					return 0;
				}
			}

			// enter next pass
			if (!passes[cmd->pass]->cls->enter(passes[cmd->pass])) {
				// TODO: push proper error to stack
				return 0;
			}
			current_pass = cmd->pass;

			// zero the storage for pass values
			memset(
				current_pass_values,
				0,
				sizeof(struct ShaderUniformValue) * MAX_UNIFORM_COUNT
			);

			// bind the shader and initialize pass values
			current_shader = passes[current_pass]->cls->get_shader(passes[current_pass]);
			shader_bind(current_shader);
		}

		// update only changed uniforms
		for (unsigned v = 0; v < cmd->values_count; v++) {
			if (current_pass_values[v].uniform != cmd->values[v].uniform ||
			    current_pass_values[v].data != cmd->values[v].data ||
			    current_pass_values[v].count != cmd->values[v].count) {
				int ok = shader_uniform_set(
					cmd->values[v].uniform,
					cmd->values[v].count,
					cmd->values[v].data
				);
				if (!ok) {
					err(ERR_GENERIC);
					return 0;
				}
				current_pass_values[v] = cmd->values[v];
			}
		}

		// bind geometry
		if (current_geom != cmd->geometry) {
			if (current_geom) {
				geometry_unbind(current_geom);
				current_geom = NULL;
			}
			if (!geometry_bind(cmd->geometry)) {
				return 0;
			}
			current_geom = cmd->geometry;
		}

		// draw!
		if (cmd->pre_exec && !cmd->pre_exec(cmd->userdata)) {
			return 0;
		}
		if (!geometry_draw(current_geom, cmd->primitive_type)) {
			return 0;
		}
		if (cmd->post_exec && !cmd->post_exec(cmd->userdata)) {
			return 0;
		}
	}

	// exit last pass
	if (current_pass != -1) {
		if (!passes[current_pass]->cls->exit(passes[current_pass])) {
			// TODO: push proper error to stack
			return 0;
		}
	}

	// trim the render queue
	queue.len = 0;

	return 1;
}

void
renderer_shutdown(void)
{
	for (int i = RENDER_PASS_COUNT; i > 0; i--) {
		int index = i - 1;
		passes[index]->cls->free(passes[index]);
		passes[index] = NULL;
	}
}

struct RenderPass*
renderer_get_pass(int pass)
{
	return passes[pass];
}

int
render_mesh(
	int render_target,
	struct Mesh *mesh,
	struct MeshProps *props,
	struct Transform *t,
	struct Light *light,
	Vec *eye
) {
	assert(mesh != NULL);
	assert(props != NULL);
	assert(t != NULL);

	// TODO: remove this function
	return 1;
}

int
render_text(
	int render_target,
	struct Text *text,
	struct TextProps *props,
	struct Transform *t
) {
	// TODO: remove this function
	return 1;
}

int
render_quad(
	int render_target,
	struct Quad *quad,
	struct QuadProps *props,
	struct Transform *t
) {
	// TODO: remove this function
	return 1;
}

int
renderer_add_command(const struct RenderCommand *cmd)
{
	assert(cmd);
	assert(cmd->geometry);
	assert(cmd->pass < RENDER_PASS_COUNT);
	assert(cmd->values_count == 0 || cmd->values != NULL);

	if (queue.len == RENDER_QUEUE_SIZE) {
		err(ERR_RENDER_QUEUE_FULL);
		return 0;
	}

	queue.commands[queue.len++] = *cmd;

	return 1;
}