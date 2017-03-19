#pragma once

#include "matlib.h"

enum {
	CAMERA_PERSPECTIVE,
	CAMERA_ORTHOGRAPHIC
};

/**
 * Camera.
 */
struct Camera {
	int type;
	Vec position;
	Qtr orientation;
	Mat view;
	Mat projection;
};

void
camera_init_perspective(
	struct Camera *camera,
	float fovy,
	float aspect,
	float near,
	float far
);

void
camera_init_orthographic(
	struct Camera *camera,
	float left,
	float right,
	float top,
	float bottom,
	float near,
	float far
);

void
camera_set_position(struct Camera *camera, const Vec *pos);

void
camera_set_orientation(struct Camera *camera, const Qtr *rot);

void
camera_look_at(struct Camera *camera, const Vec *eye, const Vec *target, const Vec *up);