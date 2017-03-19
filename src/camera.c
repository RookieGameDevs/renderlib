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
	camera->orientation = qtr(1, 0, 0, 0);
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
	camera->orientation = qtr(1, 0, 0, 0);
	mat_ident(&camera->view);
	mat_ortho(&camera->projection, left, right, top, bottom, near, far);
}

static void
update_view_matrix(struct Camera *cam)
{
	mat_ident(&cam->view);

	// translate
	Vec v = cam->position;
	vec_imulf(&v, -1);
	mat_translatev(&cam->view, &v);

	// rotate
	mat_rotateq(&cam->view, &cam->orientation);
}

void
camera_set_position(struct Camera *camera, const Vec *pos)
{
	assert(camera != NULL);
	assert(pos != NULL);

	camera->position = *pos;
	update_view_matrix(camera);
}

void
camera_set_orientation(struct Camera *camera, const Qtr *rot)
{
	assert(camera != NULL);
	assert(rot != NULL);

	camera->orientation = *rot;
	update_view_matrix(camera);
}

void
camera_look_at(struct Camera *camera, const Vec *eye, const Vec *target, const Vec *up)
{
	assert(camera != NULL);
	assert(target != NULL);

	Mat look;
	mat_lookatv(&look, eye, target, up);
	camera->orientation = mat_get_rotation(&look);

	update_view_matrix(camera);
}