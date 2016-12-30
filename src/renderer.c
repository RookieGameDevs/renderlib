#include "renderer.h"
#include <GL/glew.h>
#include <assert.h>

// defined in draw.c
int
init_mesh_pipeline(err_t *r_err);

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
renderer_present(void)
{
	return 1;
}

void
renderer_shutdown(void)
{
	// TODO
}
