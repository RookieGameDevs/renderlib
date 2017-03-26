#include "camera.h"
#include <assert.h>
#include <math.h>
#include <stddef.h>

#define assert_float_ne(a, b) assert(fabs(a - b) > 1e-3)

void
camera_init_perspective(
	struct Camera *camera,
	float fovy,
	float aspect,
	float near,
	float far
) {
	assert(camera != NULL);
	assert(fovy > 0);
	assert(aspect > 0);
	assert(near > 0);
	assert(far > 0);
	assert(near < far);
	assert_float_ne(near, far);

	camera->type = CAMERA_ORTHOGRAPHIC;
	camera->position = vec(0, 0, 0, 0);
	mat_ident(&camera->view);
	mat_persp(&camera->projection, fovy, aspect, near, far);
}

void
camera_init_orthographic(
	struct Camera *camera,
	float left,
	float right,
	float top,
	float bottom,
	float near,
	float far
) {
	assert(camera != NULL);
	assert_float_ne(left, right);
	assert_float_ne(top, bottom);
	assert(near >= 0);
	assert(near < far);
	assert_float_ne(near, far);

	camera->type = CAMERA_ORTHOGRAPHIC;
	camera->position = vec(0, 0, 0, 0);
	mat_ident(&camera->view);
	mat_ortho(&camera->projection, left, right, top, bottom, near, far);
}
