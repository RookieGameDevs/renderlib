#include "error.h"
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
init_mesh_pipeline(void);

int
draw_mesh(struct Mesh *mesh, struct MeshRenderProps *props);

static int
render_queue_push(const struct RenderOp *op)
{
	if (render_queue.len == RENDER_QUEUE_SIZE) {
		err(ERR_RENDER_QUEUE_FULL);
		return 0;
	}
	render_queue.queue[render_queue.len++] = *op;
	return 1;
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

	// initialize pipelines
	if (!init_mesh_pipeline()) {
		renderer_shutdown();
		return 0;
	}

	return 1;
}

int
renderer_present(void)
{
	int ok = render_queue_exec();
	render_queue_flush();
	return ok;
}

void
renderer_shutdown(void)
{
	// TODO
}

int
render_mesh(struct Mesh *mesh, struct MeshRenderProps *props) {
	struct RenderOp op = {
		.mesh = mesh,
		.props = *props
	};
	return render_queue_push(&op);
}