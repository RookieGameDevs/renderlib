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
