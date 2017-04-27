#include "renderlib.h"
#include "shadow_map.h"
#include <GL/glew.h>
#include <assert.h>
#include <stdlib.h>
#include <string.h>

#define RENDER_QUEUE_SIZE 1000

#define MAX_UNIFORM_COUNT 1024

// defined in draw_mesh.c
int
init_mesh_pipeline(void);

int
draw_mesh(
	struct Mesh *mesh,
	struct MeshProps *props,
	struct Transform *transform,
	struct Light *light,
	Vec *eye,
	int shadow_map
);

// defined in draw_shadow.c
int
init_shadow_pipeline(void);

int
draw_mesh_shadow(
	struct Mesh *mesh,
	struct MeshProps *props,
	struct Transform *transform,
	struct Light *light
);

// defined in draw_text.c
int
init_text_pipeline(void);

int
draw_text(struct Text *text, struct TextProps *props, struct Transform *transform);

// defined in draw_quad.c
int
init_quad_pipeline(void);

int
draw_quad(struct Quad *quad, struct QuadProps *props, struct Transform *transform);

enum {
	MESH_OP = 1,
	TEXT_OP,
	QUAD_OP
};

enum {
	PASS_SHADOW = 1,
	PASS_RENDER,
};

/*
struct RenderOp {
	int pass;
	int type;
	Vec position;
	struct Transform transform;
	union {
		struct {
			struct Mesh *mesh;
			struct MeshProps props;
			struct Light light;
			Vec eye;
			int is_lit;
		} mesh;
		struct {
			struct Text *text;
			struct TextProps props;
		} text;
		struct {
			struct Quad *quad;
			struct QuadProps props;
		} quad;
	};
	int (*exec)(struct RenderOp *op);
};
*/

/** Shadow pass functions **/
extern int
shadow_pass_init(void);

extern void
shadow_pass_cleanup(void);

extern int
shadow_pass_enter(void);

extern int
shadow_pass_exit(void);

extern struct Shader*
shadow_pass_get_shader(void);

/** Text pass functions **/
extern int
text_pass_init(void);

extern void
text_pass_cleanup(void);

extern int
text_pass_enter(void);

extern int
text_pass_exit(void);

extern struct Shader*
text_pass_get_shader(void);

/** Quad pass functions **/
extern int
quad_pass_init(void);

extern void
quad_pass_cleanup(void);

extern int
quad_pass_enter(void);

extern int
quad_pass_exit(void);

extern struct Shader*
quad_pass_get_shader(void);

/** Phong pass functions **/
extern int
phong_pass_init(void);

extern void
phong_pass_cleanup(void);

extern int
phong_pass_enter(void);

extern int
phong_pass_exit(void);

extern struct Shader*
phong_pass_get_shader(void);

static struct RenderPass {
	const char *name;

	int
	(*init)(void);

	void
	(*cleanup)(void);

	int
	(*enter)(void);

	int
	(*exit)(void);

	struct Shader*
	(*get_shader)(void);
} passes[] = {
	{
		"shadow",
		shadow_pass_init,
		shadow_pass_cleanup,
		shadow_pass_enter,
		shadow_pass_exit,
		shadow_pass_get_shader
	},
	{
		"text",
		text_pass_init,
		text_pass_cleanup,
		text_pass_enter,
		text_pass_exit,
		text_pass_get_shader
	},
	{
		"quad",
		quad_pass_init,
		quad_pass_cleanup,
		quad_pass_enter,
		quad_pass_exit,
		quad_pass_get_shader
	},
	{
		"phong",
		phong_pass_init,
		phong_pass_cleanup,
		phong_pass_enter,
		phong_pass_exit,
		phong_pass_get_shader
	}
};

#define RENDER_PASS_COUNT (sizeof(passes) / sizeof(struct RenderPass))

struct DrawCommand {
	int pass;
	struct Geometry *geometry;
	struct ShaderUniformValue *values;
	size_t value_count;
};

static struct DrawList {
	struct DrawCommand commands[RENDER_QUEUE_SIZE];
	size_t len;
} draw_list = { .len = 0 };

static struct ShaderUniformValue *current_pass_values = NULL;

static int
draw_command_cmp(const void *a_ptr, const void *b_ptr)
{
	const struct DrawCommand *a = a_ptr, *b = b_ptr;
	int pass_diff = a->pass - b->pass;
	if (pass_diff != 0) {
		return pass_diff;
	}
	return a->geometry - b->geometry;
}

/*
static struct RenderQueue {
	struct RenderOp queue[RENDER_QUEUE_SIZE];
	size_t len;
} shadow_queue = { .len = 0 }, render_queue = { .len = 0 }, overlay_queue = { .len = 0 };

static struct ShadowMap *shadow_map = NULL;
static int shadow_map_tu = -1;

static int
render_queue_push(struct RenderQueue *q, const struct RenderOp *op)
{
	if (q->len == RENDER_QUEUE_SIZE) {
		err(ERR_RENDER_QUEUE_FULL);
		return 0;
	}
	q->queue[q->len++] = *op;
	return 1;
}

static void
render_queue_flush(struct RenderQueue *q)
{
	q->len = 0;
}

static int
exec_mesh_op(struct RenderOp *op)
{
	int ok = 1;
	switch (op->pass) {
	case PASS_SHADOW:
		ok &= draw_mesh_shadow(
			op->mesh.mesh,
			&op->mesh.props,
			&op->transform,
			&op->mesh.light
		);
		break;
	case PASS_RENDER:
		ok &= draw_mesh(
			op->mesh.mesh,
			&op->mesh.props,
			&op->transform,
			op->mesh.is_lit ? &op->mesh.light : NULL,
			op->mesh.is_lit ? &op->mesh.eye : NULL,
			shadow_map_tu
		);
		break;
	}
	return ok;
}

static int
exec_text_op(struct RenderOp *op)
{
	return draw_text(op->text.text, &op->text.props, &op->transform);
}

static int
exec_quad_op(struct RenderOp *op)
{
	return draw_quad(op->quad.quad, &op->quad.props, &op->transform);
}

static int
render_op_cmp(const void *ptr1, const void *ptr2)
{
	const struct RenderOp *op1 = ptr1, *op2 = ptr2;

	// meshes come first as they are non-transparent non-translucent opaque
	// objects by definition
	if (op1->type == MESH_OP && op2->type == MESH_OP) {
		if (op1->mesh.mesh < op2->mesh.mesh) {
			return -1;
		} else if (op1->mesh.mesh == op2->mesh.mesh) {
			return 0;
		}
		return 1;
	} else if (op1->type == MESH_OP) {
		return -1;
	} else if (op2->type == MESH_OP) {
		return 1;
	}

	// TODO: this is a simple Z-based sort, which works only for text and
	// quads which are rendered using a non-rotated orthographic projection
	// volume
	if (op1->position.data[2] < op2->position.data[2]) {
		return -1;
	} else if (fabs(op1->position.data[2] - op2->position.data[2]) < 1e-6) {
		return 0;
	}
	return 1;
}

static int
render_queue_exec(struct RenderQueue *q)
{
	int ok = 1;

	// compute op target position in world coordinates
	Mat modelview;
	Vec origin = vec(0, 0, 0, 1);
	for (size_t i = 0; i < q->len; i++) {
		mat_mul(
			&q->queue[i].transform.view,
			&q->queue[i].transform.model,
			&modelview
		);
		mat_mulv(&modelview, &origin, &q->queue[i].position);
	}

	// sort operations in render queue as specified by `render_op_cmp()`
	qsort(q->queue, q->len, sizeof(struct RenderOp), render_op_cmp);

	// execute render operations
	for (int i = 0; i < q->len; i++) {
		struct RenderOp *op = &q->queue[i];
		ok &= op->exec(op);
	}
	return ok;
}
*/

int
renderer_init(void)
{
	// initialize GLEW
	glewExperimental = GL_TRUE;
	if (glewInit() != 0) {
		err(ERR_GLEW);
		return 0;
	}
	// silence any errors produced during GLEW initialization
	glGetError();

	// one-off OpenGL initializations
	glClearColor(0.3, 0.3, 0.3, 1.0);
	glEnable(GL_DEPTH_TEST);

	// initialize pipelines
	/*
	if (!init_mesh_pipeline() ||
	    !init_shadow_pipeline() ||
	    !init_text_pipeline() ||
	    !init_quad_pipeline()) {
		errf(ERR_GENERIC, "pipelines initialization failed");
		renderer_shutdown();
		return 0;
	}
	*/

	// initialize render passes
	for (unsigned i = 0; i < RENDER_PASS_COUNT; i++) {
		if (!passes[i].init()) {
			errf(
				ERR_GENERIC,
				"%s pass initialization failed",
				passes[i].name
			);
			goto error;
		}
	}

	/*
	// create shadow map
	if (!(shadow_map = shadow_map_new(1024, 1024))) {
		errf(ERR_GENERIC, "shadow map creation failed");
		renderer_shutdown();
		return 0;
	}

	// reserve a texture unit for shadow map
	glGetIntegerv(
		GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS,
		&shadow_map_tu
	);
	shadow_map_tu -= 1;
	*/

	// pre-allocate an array for render pass values
	current_pass_values = malloc(
		sizeof(struct ShaderUniformValue) * MAX_UNIFORM_COUNT
	);
	if (!current_pass_values) {
		err(ERR_NO_MEM);
		goto error;
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

/*
int
renderer_present(void)
{
	int ok = 1;

	// shadows pass
	int viewport[4];
	glGetIntegerv(GL_VIEWPORT, viewport);
	glViewport(0, 0, shadow_map->width, shadow_map->height);
	glBindFramebuffer(GL_FRAMEBUFFER, shadow_map->fbo);
	glClear(GL_DEPTH_BUFFER_BIT);
	ok = render_queue_exec(&shadow_queue);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glViewport(viewport[0], viewport[1], viewport[2], viewport[3]);
	if (!ok) {
		errf(ERR_GENERIC, "shadow pass failed");
		goto cleanup;
	}

	// render pass
	glActiveTexture(GL_TEXTURE0 + shadow_map_tu);
	glBindTexture(GL_TEXTURE_2D, shadow_map->texture);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	ok = render_queue_exec(&render_queue);
	glBindTexture(GL_TEXTURE_2D, 0);
	if (!ok) {
		errf(ERR_GENERIC, "render pass failed");
		goto cleanup;
	}

	// overlay pass
	glClear(GL_DEPTH_BUFFER_BIT);
	glDisable(GL_DEPTH_TEST);
	ok = render_queue_exec(&overlay_queue);
	glEnable(GL_DEPTH_TEST);
	if (!ok) {
		errf(ERR_GENERIC, "overlay pass failed");
		goto cleanup;
	}

cleanup:
	render_queue_flush(&shadow_queue);
	render_queue_flush(&render_queue);
	render_queue_flush(&overlay_queue);

	return ok;
}
*/

int
renderer_present(void)
{
	// sort the draw commands list
	qsort(
		draw_list.commands,
		draw_list.len,
		sizeof(struct DrawCommand),
		draw_command_cmp
	);

	// perform actual draw
	int current_pass = -1;
	struct Shader *current_shader = NULL;
	struct Geometry *current_geom = NULL;
	for (size_t i = 0; i < draw_list.len; i++) {
		struct DrawCommand *cmd = &draw_list.commands[i];

		// switch to next render pass, if needed
		if (current_pass != cmd->pass) {
			// exit current pass
			if (current_pass != -1) {
				if (!passes[current_pass].exit()) {
					// TODO: push proper error to stack
					return 0;
				}
			}

			current_pass = cmd->pass;

			// enter next pass
			if (!passes[current_pass].enter()) {
				// TODO: push proper error to stack
				return 0;
			}

			// zero the storage for pass values
			memset(
				current_pass_values,
				0,
				sizeof(struct ShaderUniformValue) * MAX_UNIFORM_COUNT
			);

			// initialize the pass values cache
			current_shader = passes[current_pass].get_shader();
			for (unsigned u = 0; u < current_shader->uniform_count; u++) {
				current_pass_values[u].uniform = &current_shader->uniforms[u];
				current_pass_values[u].count = 0;
				current_pass_values[u].data = NULL;
			}
		}

		// update only changed uniforms
		for (unsigned u = 0; u < current_shader->uniform_count; u++) {
			for (unsigned v = 0; v < cmd->value_count; v++) {
				if (current_pass_values[u].uniform != cmd->values[v].uniform) {
					continue;
				}

				if (current_pass_values[u].data != cmd->values[v].data ||
				    current_pass_values[u].count != cmd->values[v].count) {
					current_pass_values[u] = cmd->values[v];
					int ok = shader_uniform_set(
						cmd->values[v].uniform,
						cmd->values[v].count,
						cmd->values[v].data
					);
					if (!ok) {
						// TODO: push proper error message
						return 0;
					}
					break;
				}
			}
		}

		// bind geometry
		if (current_geom != cmd->geometry) {
			glBindVertexArray(cmd->geometry->vao);
			current_geom = cmd->geometry;
		}

		// draw!
		glDrawArrays(GL_TRIANGLES, 0, 3);
		if (glGetError() != GL_NO_ERROR) {
			err(ERR_OPENGL);
			break;
		}
	}

	// exit last pass
	if (current_pass != -1) {
		if (!passes[current_pass].exit()) {
			// TODO: push proper error to stack
			return 0;
		}
	}

	// trim the draw list
	draw_list.len = 0;

	return 1;
}

void
renderer_shutdown(void)
{
	free(current_pass_values);
	current_pass_values = 0;

	for (int i = RENDER_PASS_COUNT; i > 0; i--) {
		passes[i - 1].cleanup();
	}

	/*
	shadow_map_free(shadow_map);
	shadow_map = NULL;
	*/
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

	int ok = 1;

	/*
	struct RenderOp op = {
		.type = MESH_OP,
		.transform = *t,
		.mesh = {
			.mesh = mesh,
			.props = *props,
		},
		.exec = exec_mesh_op
	};

	// enable lighting and shadow casting only if light parameters are
	// specified
	if (light && eye && render_target == RENDER_TARGET_FRAMEBUFFER) {
		op.mesh.is_lit = 1;
		op.mesh.light = *light;
		op.mesh.eye = *eye;

		// shadow pass
		if (props->cast_shadows) {
			op.pass = PASS_SHADOW;
			ok &= render_queue_push(&shadow_queue, &op);
		}
	}

	// render pass
	op.pass = PASS_RENDER;
	if (render_target == RENDER_TARGET_FRAMEBUFFER) {
		ok &= render_queue_push(&render_queue, &op);
	} else {
		ok &= render_queue_push(&overlay_queue, &op);
	}
	*/

	return ok;
}

int
render_text(
	int render_target,
	struct Text *text,
	struct TextProps *props,
	struct Transform *t
) {
	/*
	struct RenderOp op = {
		.type = TEXT_OP,
		.transform = *t,
		.pass = PASS_RENDER,
		.text = {
			.text = text,
			.props = *props
		},
		.exec = exec_text_op
	};
	if (render_target == RENDER_TARGET_FRAMEBUFFER) {
		return render_queue_push(&render_queue, &op);
	}
	return render_queue_push(&overlay_queue, &op);
	*/
	return 1;
}

int
render_quad(
	int render_target,
	struct Quad *quad,
	struct QuadProps *props,
	struct Transform *t
) {
	/*
	struct RenderOp op = {
		.type = QUAD_OP,
		.transform = *t,
		.pass = PASS_RENDER,
		.quad = {
			.quad = quad,
			.props = *props
		},
		.exec = exec_quad_op
	};
	if (render_target == RENDER_TARGET_FRAMEBUFFER) {
		return render_queue_push(&render_queue, &op);
	}
	return render_queue_push(&overlay_queue, &op);
	*/
	return 1;
}

struct Shader*
renderer_get_shader(int pass)
{
	if (pass >= 0 && pass < RENDER_PASS_COUNT) {
		return passes[pass].get_shader();
	}
	return NULL;
}

int
renderer_draw(
	struct Geometry *geom,
	int pass,
	struct ShaderUniformValue *values,
	size_t value_count
) {
	assert(geom);
	assert(pass < RENDER_PASS_COUNT);
	assert(values != NULL);

	if (draw_list.len == RENDER_QUEUE_SIZE) {
		err(ERR_RENDER_QUEUE_FULL);
		return 0;
	}
	struct DrawCommand *cmd = &draw_list.commands[draw_list.len++];
	cmd->pass = pass;
	cmd->geometry = geom;
	cmd->values = values;
	cmd->value_count = value_count;
	return 1;
}