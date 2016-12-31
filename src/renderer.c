#include "renderer.h"
#include <GL/glew.h>
#include <assert.h>

#define RENDER_QUEUE_SIZE 1000

struct RenderOp {
	struct Mesh *mesh;
	struct MeshRenderProps props;
};

static struct RenderQueue {
	struct RenderOp queue[RENDER_QUEUE_SIZE];
	size_t len;
} render_queue;

// defined in draw.c
int
init_mesh_pipeline(err_t *r_err);

int
draw_mesh(struct Mesh *mesh, struct MeshRenderProps *props);

static int
render_queue_push(const struct RenderOp *op)
{
	if (render_queue.len < RENDER_QUEUE_SIZE) {
		render_queue.queue[render_queue.len++] = *op;
		return 1;
	}
	return 0;
}

static void
render_queue_flush(void)
{
	render_queue.len = 0;
}

static int
render_queue_exec(void)
{
	for (int i = 0; i < render_queue.len; i++) {
		struct RenderOp *op = &render_queue.queue[i];
		if (!draw_mesh(op->mesh, &op->props)) {
			return 0;
		}
	}
	return 1;
}

int
renderer_init(err_t *r_err)
{
	err_t err = 0;

	// initialize GLEW
	glewExperimental = GL_TRUE;
	if (glewInit() != 0) {
		err = ERR_GLEW;
		goto error;
	}
	// silence any errors produced during GLEW initialization
	glGetError();

	// initialize pipelines
	int ok = (
		init_mesh_pipeline(&err)
	);
	if (!ok) {
		goto error;
	}

	return 1;

error:
	if (r_err) {
		*r_err = err;
	}
	renderer_shutdown();
	return 0;
}

int
renderer_present(err_t *r_err)
{
	err_t err = 0;
	if (!render_queue_exec()) {
		err = ERR_RENDER;
	}
	render_queue_flush();

	if (err && r_err) {
		*r_err = err;
	}
	return err == 0;
}

void
renderer_shutdown(void)
{
	// TODO
}

int
render_mesh(struct Mesh *mesh, struct MeshRenderProps *props, err_t *r_err) {
	struct RenderOp op = {
		.mesh = mesh,
		.props = *props
	};
	if (!render_queue_push(&op)) {
		if (r_err) {
			*r_err = ERR_RENDER_QUEUE_FULL;
		}
		return 0;
	}
	return 1;
}