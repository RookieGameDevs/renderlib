#include "renderlib.h"
#include "shadow_map.h"
#include <GL/glew.h>
#include <assert.h>

#define RENDER_QUEUE_SIZE 1000

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
	SHADOW_PASS = 1,
	RENDER_PASS
};

struct RenderOp {
	int pass;
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

static struct RenderQueue {
	struct RenderOp queue[RENDER_QUEUE_SIZE];
	size_t len;
} shadow_queue = { .len = 0 }, render_queue = { .len = 0 };

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
	case SHADOW_PASS:
		ok &= draw_mesh_shadow(
			op->mesh.mesh,
			&op->mesh.props,
			&op->transform,
			&op->mesh.light
		);
		break;
	case RENDER_PASS:
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
render_queue_exec(struct RenderQueue *q)
{
	int ok = 1;
	for (int i = 0; i < q->len; i++) {
		struct RenderOp *op = &q->queue[i];
		ok &= op->exec(op);
	}
	return ok;
}

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
	if (!init_mesh_pipeline() ||
	    !init_shadow_pipeline() ||
	    !init_text_pipeline() ||
	    !init_quad_pipeline()) {
		errf(ERR_GENERIC, "pipelines initialization failed");
		renderer_shutdown();
		return 0;
	}

	// create shadow map
	if (!(shadow_map = shadow_map_new(1024, 1024))) {
		errf(ERR_GENERIC, "shadow map creation failed");
		return 0;
	}

	// reserve a texture unit for shadow map
	glGetIntegerv(
		GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS,
		&shadow_map_tu
	);
	shadow_map_tu -= 1;

	return 1;
}

void
renderer_clear(void)
{
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

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

cleanup:
	render_queue_flush(&shadow_queue);
	render_queue_flush(&render_queue);

	return ok;
}

void
renderer_shutdown(void)
{
	shadow_map_free(shadow_map);
	shadow_map = NULL;
}

int
render_mesh(
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

	struct RenderOp op = {
		.transform = *t,
		.mesh = {
			.mesh = mesh,
			.props = *props,
		},
		.exec = exec_mesh_op
	};

	// enable lighting and shadow casting only if light parameters are
	// specified
	if (light && eye) {
		op.mesh.is_lit = 1;
		op.mesh.light = *light;
		op.mesh.eye = *eye;

		// shadow pass
		if (props->cast_shadows) {
			op.pass = SHADOW_PASS;
			ok &= render_queue_push(&shadow_queue, &op);
		}
	}

	// render pass
	op.pass = RENDER_PASS;
	ok &= render_queue_push(&render_queue, &op);

	return ok;
}

int
render_text(struct Text *text, struct TextProps *props, struct Transform *t)
{
	struct RenderOp op = {
		.transform = *t,
		.pass = RENDER_PASS,
		.text = {
			.text = text,
			.props = *props
		},
		.exec = exec_text_op
	};
	return render_queue_push(&render_queue, &op);
}

int
render_quad(struct Quad *quad, struct QuadProps *props, struct Transform *t)
{
	struct RenderOp op = {
		.transform = *t,
		.pass = RENDER_PASS,
		.quad = {
			.quad = quad,
			.props = *props
		},
		.exec = exec_quad_op
	};
	return render_queue_push(&render_queue, &op);
}